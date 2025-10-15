// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "gin/handle.h"
#include "shell/browser/api/super_cr_view.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_helper/event_emitter.h"
#include "ui/gfx/geometry/transform.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"
#include "v8/include/v8-value.h"

namespace electron::api {

using ChildPair = std::pair<raw_ptr<views::View>, v8::Global<v8::Object>>;

class View : public gin_helper::EventEmitter<View>,
             public super::SuperCrViewDelegates,
             private views::ViewObserver {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);
  static gin::Handle<View> Create(v8::Isolate* isolate);

  // Return the cached constructor function.
  static v8::Local<v8::Function> GetConstructor(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void AddChildViewAt(gin::Handle<View> child, std::optional<size_t> index);
  void RemoveChildView(gin::Handle<View> child);

  void SetBounds(const gfx::Rect& bounds);
  gfx::Rect GetBounds();
  void SetLayout(v8::Isolate* isolate, v8::Local<v8::Object> value);
  std::vector<v8::Local<v8::Value>> GetChildren();
  void SetBackgroundColor(std::optional<WrappedSkColor> color);
  void SetBorderRadius(int radius);
  void SetVisible(bool visible);

  // views::ViewObserver
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewIsDeleting(views::View* observed_view) override;
  void OnChildViewRemoved(views::View* observed_view,
                          views::View* child) override;
  bool HitTestPoint(const gfx::Point& point) const;

  views::View* view() const { return view_; }
  std::optional<int> border_radius() const { return border_radius_; }

  // disable copy
  View(const View&) = delete;
  View& operator=(const View&) = delete;

  gfx::Size GetPreferredSize(const views::SizeBounds& available_size) const;
  gfx::Insets GetInsets() const;
  void SizeToPreferredSize();

  void SetPreferredSize(gfx::Size size);

  void LayoutImmediately();
  void InvalidateLayout();

  void SetNotifyEnterExitOnChild(bool notify);
  bool GetNotifyEnterExitOnChild() const;

 protected:
  explicit View(views::View* view);
  View();
  ~View() override;

  // Should delete the |view_| in destructor.
  void set_delete_view(bool should) { delete_view_ = should; }

  void OnCrMouseEntered(const ui::MouseEvent& event) override;
  void OnCrMouseExited(const ui::MouseEvent& event) override;
  void OnCrMouseMoved(const ui::MouseEvent& event) override;
  bool OnCrMousePressed(const ui::MouseEvent& event) override;
  void OnCrMouseReleased(const ui::MouseEvent& event) override;
  bool OnCrMouseDragged(const ui::MouseEvent& event) override;
  void OnCrMouseCaptureLost() override;
  void SetPaintToLayer();
  void SetTransform(const gfx::Transform& transform);
  gfx::Transform GetTransform() const;
  void SetLayerFillsBoundsOpaquely(bool isOpaque);
  void SetLayerColor(std::optional<WrappedSkColor> color);

 private:
  void ApplyBorderRadius();
  void ReorderChildView(gin::Handle<View> child, size_t index);

  std::vector<ChildPair> child_views_;
  std::optional<int> border_radius_;

  bool delete_view_ = true;
  raw_ptr<views::View> view_ = nullptr;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_
