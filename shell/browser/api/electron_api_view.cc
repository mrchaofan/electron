// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_view.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "ash/style/rounded_rect_cutout_path_builder.h"
#include "gin/data_object_builder.h"
#include "gin/wrappable.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/ui/layout_event_forwarding_view.h"
#include "shell/browser/ui/mouse_event_forwarding_view.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/optional_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/gfx/geometry/transform.h"
#include "ui/gfx/shadow_value.h"
#include "ui/views/animation/animation_builder.h"
#include "ui/views/animation/ink_drop_painted_layer_delegates.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_manager_base.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view_targeter.h"
#include "ui/views/view_targeter_delegate.h"

#if BUILDFLAG(IS_MAC)
#include "shell/browser/animation_util.h"
#endif

namespace gin {

template <>
struct Converter<views::ChildLayout> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::ChildLayout* out) {
    gin_helper::Dictionary dict;
    if (!gin::ConvertFromV8(isolate, val, &dict))
      return false;
    gin_helper::Handle<electron::api::View> view;
    if (!dict.Get("view", &view))
      return false;
    out->child_view = view->view();
    if (dict.Has("bounds"))
      dict.Get("bounds", &out->bounds);
    out->visible = true;
    if (dict.Has("visible"))
      dict.Get("visible", &out->visible);
    return true;
  }
};

template <>
struct Converter<views::ProposedLayout> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::ProposedLayout* out) {
    gin_helper::Dictionary dict;
    if (!gin::ConvertFromV8(isolate, val, &dict))
      return false;
    if (!dict.Get("size", &out->host_size))
      return false;
    if (!dict.Get("layouts", &out->child_layouts))
      return false;
    return true;
  }
};

template <>
struct Converter<views::LayoutOrientation> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::LayoutOrientation* out) {
    std::string orientation = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (orientation == "horizontal") {
      *out = views::LayoutOrientation::kHorizontal;
    } else if (orientation == "vertical") {
      *out = views::LayoutOrientation::kVertical;
    } else {
      return false;
    }
    return true;
  }
};

template <>
struct Converter<views::LayoutAlignment> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::LayoutAlignment* out) {
    std::string orientation = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (orientation == "start") {
      *out = views::LayoutAlignment::kStart;
    } else if (orientation == "center") {
      *out = views::LayoutAlignment::kCenter;
    } else if (orientation == "end") {
      *out = views::LayoutAlignment::kEnd;
    } else if (orientation == "stretch") {
      *out = views::LayoutAlignment::kStretch;
    } else if (orientation == "baseline") {
      *out = views::LayoutAlignment::kBaseline;
    } else {
      return false;
    }
    return true;
  }
};

template <>
struct Converter<views::FlexAllocationOrder> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::FlexAllocationOrder* out) {
    std::string orientation = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (orientation == "normal") {
      *out = views::FlexAllocationOrder::kNormal;
    } else if (orientation == "reverse") {
      *out = views::FlexAllocationOrder::kReverse;
    } else {
      return false;
    }
    return true;
  }
};

template <>
struct Converter<views::SizeBound> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const views::SizeBound& in) {
    if (in.is_bounded())
      return v8::Integer::New(isolate, in.value());
    return v8::Number::New(isolate, std::numeric_limits<double>::infinity());
  }
};

template <>
struct Converter<views::SizeBounds> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const views::SizeBounds& in) {
    return gin::DataObjectBuilder(isolate)
        .Set("width", in.width())
        .Set("height", in.height())
        .Build();
  }
};

template <>
struct Converter<gfx::Tween::Type> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Tween::Type* out) {
    std::string easing = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (easing == "linear") {
      *out = gfx::Tween::LINEAR;
    } else if (easing == "ease-in") {
      *out = gfx::Tween::EASE_IN;
    } else if (easing == "ease-out") {
      *out = gfx::Tween::EASE_OUT;
    } else if (easing == "ease-in-out") {
      *out = gfx::Tween::EASE_IN_OUT;
    } else {
      return false;
    }
    return true;
  }
};
}  // namespace gin

namespace electron::api {

using LayoutCallback = base::RepeatingCallback<views::ProposedLayout(
    const views::SizeBounds& size_bounds)>;

class JSLayoutManager : public views::LayoutManagerBase {
 public:
  explicit JSLayoutManager(LayoutCallback layout_callback)
      : layout_callback_(std::move(layout_callback)) {}
  ~JSLayoutManager() override = default;

  // views::LayoutManagerBase
  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    return layout_callback_.Run(size_bounds);
  }

 private:
  LayoutCallback layout_callback_;
};

class NativeView : public views::View,
                   public electron::MouseEventForwardingView,
                   public electron::LayoutEventForwardingView,
                   public views::ViewTargeterDelegate {
 public:
  explicit NativeView(electron::api::View* delegate)
      : MouseEventForwardingView(delegate),
        LayoutEventForwardingView(delegate) {
    SetEventTargeter(std::make_unique<views::ViewTargeter>(this));
  }

  bool DoesIntersectRect(const views::View* target,
                         const gfx::Rect& rect) const override {
    DCHECK_EQ(this, target);
    if (!views::ViewTargeterDelegate::DoesIntersectRect(target, rect))
      return false;
    if (rect.width() != 1 || rect.height() != 1)
      return true;
    return ForwardHitTestPoint(rect.origin());
  }

  bool OnMousePressed(const ui::MouseEvent& event) override {
    return ForwardMousePressed(event);
  }

  bool OnMouseDragged(const ui::MouseEvent& event) override {
    return ForwardMouseDragged(event);
  }

  void OnMouseReleased(const ui::MouseEvent& event) override {
    ForwardMouseReleased(event);
  }

  void OnMouseCaptureLost() override { ForwardMouseCaptureLost(); }

  void OnMouseMoved(const ui::MouseEvent& event) override {
    ForwardMouseMoved(event);
  }

  void OnMouseEntered(const ui::MouseEvent& event) override {
    ForwardMouseEntered(event);
  }

  void OnMouseExited(const ui::MouseEvent& event) override {
    ForwardMouseExited(event);
  }

  bool OnMouseWheel(const ui::MouseWheelEvent& event) override {
    return ForwardMouseWheel(event);
  }

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override {
    auto preferred_size = ForwardCalculatePreferredSize(available_size);
    return preferred_size.value_or(gfx::Size());
  }

  void Layout(PassKey) override {
    LayoutEventForwardingView::OnLayout();
    LayoutSuperclass<views::View>(this);
  }
};

void SetMouseEventProperties(gin_helper::Dictionary* event_dict,
                             const ui::MouseEvent& event) {
  event_dict->Set("location", event.location_f());
  event_dict->Set("rootLocation", event.root_location_f());
  event_dict->Set("flags", event.flags());
  event_dict->Set("changedButtonFlags", event.changed_button_flags());
  event_dict->Set("clickCount", event.GetClickCount());
}

void SetMouseWheelEventProperties(gin_helper::Dictionary* event_dict,
                                  const ui::MouseWheelEvent& event) {
  SetMouseEventProperties(event_dict, event);
  event_dict->Set("offsetX", event.x_offset());
  event_dict->Set("offsetY", event.y_offset());
}

// |default_return_value| determines both whether the JavaScript event exposes
// returnValue and its initial value. The result is returned to the
// corresponding Chromium callback after applying preventDefault().
template <typename PopulateEventData>
bool EmitUnifiedEventObject(View* view,
                            std::string_view name,
                            std::optional<bool> default_return_value,
                            PopulateEventData populate_event_data) {
  v8::Isolate* isolate = view->isolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Object> wrapper = view->GetWrapper();
  if (wrapper.IsEmpty())
    return default_return_value.value_or(false);

  gin_helper::internal::Event* js_event =
      gin_helper::internal::Event::New(isolate);
  v8::Local<v8::Object> event_object =
      js_event->GetWrapper(isolate).ToLocalChecked();

  gin_helper::Dictionary event_dict(isolate, event_object);
  if (default_return_value.has_value())
    event_dict.Set("returnValue", *default_return_value);

  populate_event_data(&event_dict);
  gin_helper::EmitEvent(isolate, wrapper, name, event_object);

  if (!default_return_value.has_value())
    return false;

  bool return_value = *default_return_value;
  event_dict.Get("returnValue", &return_value);
  return return_value && !js_event->GetDefaultPrevented();
}

View::View(views::View* view) {
  InitializeView(view);
}

View::View() {
  InitializeView(new NativeView(this));
}

View::~View() {
  if (!view_)
    return;
  ClearShadow();
  view_->RemoveObserver(this);
  if (delete_view_)
    view_.ClearAndDelete();
}

void View::InitializeView(views::View* view) {
  view_ = view;
  view_->set_owned_by_client(views::View::OwnedByClientPassKey{});
  view_->AddObserver(this);
  view_->SetNotifyEnterExitOnChild(true);
}

void View::ReorderChildView(gin_helper::Handle<View> child, size_t index) {
  view_->ReorderChildView(child->view(), index);

  const auto i =
      std::ranges::find_if(child_views_, [&](const ChildPair& child_view) {
        return child_view.first == child->view();
      });
  DCHECK(i != child_views_.end());

  // If |view| is already at the desired position, there's nothing to do.
  const auto pos = std::next(
      child_views_.begin(),
      static_cast<ptrdiff_t>(std::min(index, child_views_.size() - 1)));
  if (i == pos)
    return;

  if (pos < i) {
    std::rotate(pos, i, std::next(i));
  } else {
    std::rotate(i, std::next(i), std::next(pos));
  }
}

void View::AddChildViewAt(gin_helper::Handle<View> child,
                          std::optional<size_t> maybe_index) {
  // TODO(nornagon): !view_ is only for supporting the weird case of
  // WebContentsView's view being deleted when the underlying WebContents is
  // destroyed (on non-Mac). We should fix that so that WebContentsView always
  // has a View, possibly a wrapper view around the underlying platform View.
  if (!view_)
    return;

  if (!child->view()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
        "Can't add a destroyed child view to a parent view");
    return;
  }

  // This will CHECK and crash in View::AddChildViewAtImpl if not handled here.
  if (view_ == child->view()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
        "A view cannot be added as its own child");
    return;
  }

  size_t index =
      std::min(child_views_.size(), maybe_index.value_or(child_views_.size()));

  // If the child is already a child of this view, just reorder it.
  // This matches the behavior of View::AddChildViewAtImpl and
  // otherwise will CHECK if the same view is added multiple times.
  if (child->view()->parent() == view_) {
    ReorderChildView(child, index);
    return;
  }

  child_views_.emplace(child_views_.begin() + index,  // index
                       child->view(),
                       v8::Global<v8::Object>(isolate(), child->GetWrapper()));
#if BUILDFLAG(IS_MAC)
  // Disable the implicit CALayer animations that happen by default when adding
  // or removing sublayers.
  // See
  // https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/CoreAnimation_guide/ReactingtoLayerChanges/ReactingtoLayerChanges.html
  // and https://github.com/electron/electron/pull/14911
  // TODO(nornagon): Disabling these CALayer animations (which are specific to
  // WebContentsView, I think) seems like this is something that remote_cocoa
  // or views should be taking care of, but isn't. This should be pushed
  // upstream.
  ScopedCAActionDisabler disable_animations;
#endif
  view_->AddChildViewAt(child->view(), index);
}

void View::RemoveChildView(gin_helper::Handle<View> child) {
  if (!view_)
    return;

  const auto it =
      std::ranges::find_if(child_views_, [&](const ChildPair& child_view) {
        return child_view.first == child->view();
      });
  if (it != child_views_.end()) {
#if BUILDFLAG(IS_MAC)
    ScopedCAActionDisabler disable_animations;
#endif
    // Remove from child_views first so that OnChildViewRemoved doesn't try to
    // remove it again
    child_views_.erase(it);
    // It's possible for the child's view to be invalid here
    // if the child's webContents was closed or destroyed.
    if (child->view())
      view_->RemoveChildView(child->view());
  }
}

ui::Layer* View::GetLayer() {
  if (!view_)
    return nullptr;

  if (view_->layer())
    return view_->layer();

  view_->SetPaintToLayer();

  ui::Layer* layer = view_->layer();

  layer->SetFillsBoundsOpaquely(false);

  return layer;
}

void View::SetBounds(const gfx::Rect& bounds, gin::Arguments* const args) {
  bool animate = false;
  int duration = 250;
  gfx::Tween::Type easing = gfx::Tween::LINEAR;

  gin_helper::Dictionary dict;
  if (args->GetNext(&dict)) {
    v8::Local<v8::Value> animate_value;

    if (dict.Get("animate", &animate_value)) {
      if (animate_value->IsBoolean()) {
        animate = animate_value->BooleanValue(isolate());
      } else {
        animate = true;

        gin_helper::Dictionary animate_dict;
        if (gin::ConvertFromV8(isolate(), animate_value, &animate_dict)) {
          animate_dict.Get("duration", &duration);
          animate_dict.Get("easing", &easing);
        }
      }
    }
  }

  if (duration < 0)
    duration = 0;

  if (!view_)
    return;

  if (!animate) {
    view_->SetBoundsRect(bounds);
    return;
  }

  ui::Layer* layer = GetLayer();

  gfx::Rect current_bounds = view_->bounds();

  if (bounds.size() == current_bounds.size()) {
    // If the size isn't changing, we can just animate the bounds directly.

    views::AnimationBuilder()
        .SetPreemptionStrategy(
            ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET)
        .OnEnded(base::BindOnce(
            [](views::View* view, const gfx::Rect& final_bounds) {
              view->SetBoundsRect(final_bounds);
            },
            view_, bounds))
        .Once()
        .SetDuration(base::Milliseconds(duration))
        .SetBounds(view_, bounds, easing);

    return;
  }

  gfx::Rect target_size = gfx::Rect(0, 0, bounds.width(), bounds.height());
  gfx::Rect max_size =
      gfx::Rect(current_bounds.x(), current_bounds.y(),
                std::max(current_bounds.width(), bounds.width()),
                std::max(current_bounds.height(), bounds.height()));

  // if the view's size is smaller than the target size, we need to set the
  // view's bounds immediatley to the new size (not position) and set the
  // layer's clip rect to animate from there.
  if (view_->width() < bounds.width() || view_->height() < bounds.height()) {
    view_->SetBoundsRect(max_size);

    if (layer) {
      layer->SetClipRect(
          gfx::Rect(0, 0, current_bounds.width(), current_bounds.height()));
    }
  }

  views::AnimationBuilder()
      .SetPreemptionStrategy(
          ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET)
      .OnEnded(base::BindOnce(
          [](views::View* view, const gfx::Rect& final_bounds,
             ui::Layer* layer) {
            view->SetBoundsRect(final_bounds);
            if (layer)
              layer->SetClipRect(gfx::Rect());
          },
          view_, bounds, layer))
      .Once()
      .SetDuration(base::Milliseconds(duration))
      .SetBounds(view_, bounds, easing)
      .SetClipRect(
          view_, target_size,
          easing);  // We have to set the clip rect independently of the
                    // bounds, because animating the bounds of the layer
                    // will not animate the underlying view's bounds.
}

gfx::Rect View::GetBounds() const {
  if (!view_)
    return {};
  return view_->bounds();
}

gfx::Rect View::GetLocalBounds() const {
  if (!view_)
    return {};
  return view_->GetLocalBounds();
}

void View::SetLayout(v8::Isolate* isolate, v8::Local<v8::Object> value) {
  if (!view_)
    return;
  gin_helper::Dictionary dict(isolate, value);
  LayoutCallback calculate_proposed_layout;
  if (dict.Get("calculateProposedLayout", &calculate_proposed_layout)) {
    view_->SetLayoutManager(std::make_unique<JSLayoutManager>(
        std::move(calculate_proposed_layout)));
  } else {
    auto* layout =
        view_->SetLayoutManager(std::make_unique<views::FlexLayout>());
    views::LayoutOrientation orientation;
    if (dict.Get("orientation", &orientation))
      layout->SetOrientation(orientation);
    views::LayoutAlignment main_axis_alignment;
    if (dict.Get("mainAxisAlignment", &main_axis_alignment))
      layout->SetMainAxisAlignment(main_axis_alignment);
    views::LayoutAlignment cross_axis_alignment;
    if (dict.Get("crossAxisAlignment", &cross_axis_alignment))
      layout->SetCrossAxisAlignment(cross_axis_alignment);
    gfx::Insets interior_margin;
    if (dict.Get("interiorMargin", &interior_margin))
      layout->SetInteriorMargin(interior_margin);
    int minimum_cross_axis_size;
    if (dict.Get("minimumCrossAxisSize", &minimum_cross_axis_size))
      layout->SetMinimumCrossAxisSize(minimum_cross_axis_size);
    bool collapse_margins;
    if (dict.Has("collapseMargins") &&
        dict.Get("collapseMargins", &collapse_margins))
      layout->SetCollapseMargins(collapse_margins);
    bool include_host_insets_in_layout;
    if (dict.Has("includeHostInsetsInLayout") &&
        dict.Get("includeHostInsetsInLayout", &include_host_insets_in_layout))
      layout->SetIncludeHostInsetsInLayout(include_host_insets_in_layout);
    bool ignore_default_main_axis_margins;
    if (dict.Has("ignoreDefaultMainAxisMargins") &&
        dict.Get("ignoreDefaultMainAxisMargins",
                 &ignore_default_main_axis_margins))
      layout->SetIgnoreDefaultMainAxisMargins(ignore_default_main_axis_margins);
    views::FlexAllocationOrder flex_allocation_order;
    if (dict.Get("flexAllocationOrder", &flex_allocation_order))
      layout->SetFlexAllocationOrder(flex_allocation_order);
  }
}

std::vector<v8::Local<v8::Value>> View::GetChildren() {
  std::vector<v8::Local<v8::Value>> ret;
  ret.reserve(child_views_.size());

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();

  for (auto& [view, global] : child_views_)
    ret.push_back(global.Get(isolate));

  return ret;
}

void View::SetBackgroundColor(std::optional<WrappedSkColor> color) {
  if (!view_)
    return;
  view_->SetBackground(color ? views::CreateSolidBackground({*color})
                             : nullptr);
}

void View::SetBorderRadius(int radius) {
  border_radius_ = radius;
  ApplyBorderRadius();
  UpdateShadowLayer();
}

void View::ApplyBorderRadius() {
  if (!border_radius_.has_value() || !view_)
    return;

  auto size = view_->bounds().size();

  // Restrict border radius to the constraints set in the path builder class.
  // If the constraints are exceeded, the builder will crash.
  int radius;
  {
    float r = border_radius_.value() * 1.f;
    r = std::min(r, size.width() / 2.f);
    r = std::min(r, size.height() / 2.f);
    r = std::max(r, 0.f);
    radius = std::floor(r);
  }

  // RoundedRectCutoutPathBuilder has a minimum size of 32 x 32.
  if (radius > 0 && size.width() >= 32 && size.height() >= 32) {
    auto builder = ash::RoundedRectCutoutPathBuilder(gfx::SizeF(size));
    builder.CornerRadius(radius);
    view_->SetClipPath(builder.Build());
  } else {
    view_->SetClipPath(SkPath());
  }
}

void View::SetMasksToBounds(bool masks_to_bounds) {
  ui::Layer* layer = GetLayer();
  if (!layer)
    return;

  layer->SetMasksToBounds(masks_to_bounds);
}

void View::SetBorder(gin::Arguments* args) {
  if (!view_)
    return;

  v8::Local<v8::Value> value;
  if (!args->GetNext(&value) || value->IsNullOrUndefined()) {
    view_->SetBorder(nullptr);
    return;
  }

  gin_helper::Dictionary dict;
  if (!gin::ConvertFromV8(isolate(), value, &dict)) {
    args->ThrowTypeError("border must be an object");
    return;
  }

  int thickness = 1;
  dict.Get("thickness", &thickness);
  if (thickness <= 0) {
    view_->SetBorder(nullptr);
    return;
  }

  float radius = 0;
  dict.Get("radius", &radius);
  radius = std::max(radius, 0.f);

  WrappedSkColor color = SK_ColorBLACK;
  dict.Get("color", &color);

  // Only a solid line style is currently supported.
  view_->SetBorder(
      radius > 0
          ? views::CreateRoundedRectBorder(thickness, radius,
                                           static_cast<SkColor>(color))
          : views::CreateSolidBorder(thickness, static_cast<SkColor>(color)));
}

void View::SetBackgroundBlur(int blur_radius) {
  if (!view_)
    return;

  if (blur_radius < 0)
    blur_radius = 0;

  ui::Layer* layer = GetLayer();

  if (!layer)
    return;

  layer->SetBackgroundBlur(blur_radius);
}

void View::SetVisible(bool visible) {
  if (!view_)
    return;
  view_->SetVisible(visible);
}

bool View::GetVisible() const {
  return view_ ? view_->GetVisible() : false;
}

gfx::Size View::GetPreferredSize() const {
  return view_ ? view_->GetPreferredSize() : gfx::Size();
}

void View::SetPreferredSize(std::optional<gfx::Size> size) {
  if (view_)
    view_->SetPreferredSize(std::move(size));
}

void View::SizeToContents() {
  if (view_)
    view_->SizeToPreferredSize();
}

void View::InvalidateLayout() {
  if (view_)
    view_->InvalidateLayout();
}

void View::SchedulePaint() {
  if (view_)
    view_->SchedulePaint();
}

void View::PreferredSizeChanged() {
  if (view_)
    view_->PreferredSizeChanged();
}

void View::SetTranslation(float x, std::optional<float> y) {
  ui::Layer* layer = GetLayer();
  if (!layer)
    return;

  gfx::Transform transform = layer->transform();
  transform.Translate(x, y.value_or(0.0f));
  layer->SetTransform(transform);
  UpdateShadowLayer();
}

void View::SetScale(float x, std::optional<float> y) {
  ui::Layer* layer = GetLayer();
  if (!layer)
    return;

  gfx::Transform transform = layer->transform();
  transform.Scale(x, y.value_or(x));
  layer->SetTransform(transform);
  UpdateShadowLayer();
}

void View::SetRotation(float degrees) {
  ui::Layer* layer = GetLayer();
  if (!layer)
    return;

  gfx::Transform transform = layer->transform();
  transform.Rotate(degrees);
  layer->SetTransform(transform);
  UpdateShadowLayer();
}

void View::ResetTransform() {
  ui::Layer* layer = GetLayer();
  if (!layer)
    return;

  layer->SetTransform(gfx::Transform());
  UpdateShadowLayer();
}

void View::SetShadow(gin::Arguments* args) {
  gin_helper::Dictionary dict;
  if (!args->GetNext(&dict)) {
    args->ThrowTypeError("shadow options must be an object");
    return;
  }

  BoxShadowOptions options;
  WrappedSkColor color = options.color;
  dict.Get("color", &color);
  options.color = color;
  dict.Get("offsetX", &options.offset_x);
  dict.Get("offsetY", &options.offset_y);
  dict.Get("blurRadius", &options.blur_radius);
  dict.Get("spreadRadius", &options.spread_radius);
  options.blur_radius = std::max(options.blur_radius, 0);
  shadow_corner_radius_ = border_radius().value_or(0);
  dict.Get("borderRadius", &shadow_corner_radius_);
  shadow_corner_radius_ = std::max(shadow_corner_radius_, 0);
  shadow_options_ = options;
  UpdateShadowLayer();
}

void View::ClearShadow() {
  if (shadow_layer_) {
    if (view_)
      view_->RemoveLayerFromRegions(shadow_layer_.get());
    shadow_layer_->set_delegate(nullptr);
    shadow_layer_.reset();
  }
  shadow_delegate_.reset();
  shadow_options_.reset();
}

void View::SetLayerOpacity(float opacity) {
  ui::Layer* layer = GetLayer();
  if (!layer)
    return;

  layer->SetOpacity(std::clamp(opacity, 0.0f, 1.0f));
}

void View::UpdateShadowLayer() {
  if (!view_ || !shadow_options_)
    return;

  const BoxShadowOptions& options = *shadow_options_;
  const int spread = options.spread_radius;
  const gfx::Rect shadowed_bounds(-spread, -spread,
                                  std::max(0, view_->width() + 2 * spread),
                                  std::max(0, view_->height() + 2 * spread));
  const int corner_radius = std::clamp(
      shadow_corner_radius_ + spread, 0,
      std::min(shadowed_bounds.width(), shadowed_bounds.height()) / 2);
  const gfx::ShadowValues shadows = {
      gfx::ShadowValue(gfx::Vector2d(options.offset_x, options.offset_y),
                       options.blur_radius, options.color)};

  if (shadow_layer_)
    shadow_layer_->set_delegate(nullptr);
  shadow_delegate_ = std::make_unique<views::BorderShadowLayerDelegate>(
      shadows, shadowed_bounds, SK_ColorTRANSPARENT, corner_radius);
  if (!shadow_layer_) {
    shadow_layer_ = std::make_unique<ui::Layer>(ui::LAYER_TEXTURED);
    shadow_layer_->SetFillsBoundsOpaquely(false);
    ui::Layer* layer = GetLayer();
    if (!layer)
      return;
    view_->AddLayerToRegion(shadow_layer_.get(), views::LayerRegion::kBelow);
  }
  shadow_layer_->set_delegate(shadow_delegate_.get());

  ui::Layer* layer = GetLayer();
  if (!layer)
    return;
  const gfx::Rect painted_bounds =
      gfx::ToEnclosingRect(shadow_delegate_->GetPaintedBounds());
  shadow_layer_->SetBounds(gfx::Rect(painted_bounds.size()) +
                           layer->bounds().OffsetFromOrigin());
  gfx::Transform transform = layer->transform();
  transform.Translate(painted_bounds.x(), painted_bounds.y());
  shadow_layer_->SetTransform(transform);
  shadow_layer_->SchedulePaint(gfx::Rect(shadow_layer_->size()));
}

void View::EmitEventObject(std::string_view name) {
  EmitUnifiedEventObject(this, name, std::nullopt,
                         [](gin_helper::Dictionary*) {});
}

void View::EmitMouseEvent(std::string_view name, const ui::MouseEvent& event) {
  EmitUnifiedEventObject(this, name, std::nullopt,
                         [&event](gin_helper::Dictionary* event_dict) {
                           SetMouseEventProperties(event_dict, event);
                         });
}

bool View::EmitMouseEventAndReturnValue(std::string_view name,
                                        const ui::MouseEvent& event,
                                        bool default_return_value) {
  return EmitUnifiedEventObject(this, name, default_return_value,
                                [&event](gin_helper::Dictionary* event_dict) {
                                  SetMouseEventProperties(event_dict, event);
                                });
}

bool View::EmitMouseWheelEventAndReturnValue(std::string_view name,
                                             const ui::MouseWheelEvent& event) {
  return EmitUnifiedEventObject(
      this, name, true, [&event](gin_helper::Dictionary* event_dict) {
        SetMouseWheelEventProperties(event_dict, event);
      });
}

bool View::EmitHitTestPointEventAndReturnValue(std::string_view name,
                                               const gfx::Point& point) {
  return EmitUnifiedEventObject(this, name, true,
                                [&point](gin_helper::Dictionary* event_dict) {
                                  event_dict->Set("location", point);
                                });
}

bool View::OnMousePressedFromNative(const ui::MouseEvent& event) {
  return EmitMouseEventAndReturnValue("mouse-pressed", event, false);
}

bool View::OnMouseDraggedFromNative(const ui::MouseEvent& event) {
  return EmitMouseEventAndReturnValue("mouse-dragged", event, false);
}

void View::OnMouseReleasedFromNative(const ui::MouseEvent& event) {
  EmitMouseEvent("mouse-released", event);
}

void View::OnMouseCaptureLostFromNative() {
  EmitEventObject("mouse-capture-lost");
}

void View::OnMouseMovedFromNative(const ui::MouseEvent& event) {
  EmitMouseEvent("mouse-moved", event);
}

void View::OnMouseEnteredFromNative(const ui::MouseEvent& event) {
  EmitMouseEvent("mouse-entered", event);
}

void View::OnMouseExitedFromNative(const ui::MouseEvent& event) {
  EmitMouseEvent("mouse-exited", event);
}

bool View::OnMouseWheelFromNative(const ui::MouseWheelEvent& event) {
  return EmitMouseWheelEventAndReturnValue("mouse-wheel", event);
}

bool View::OnHitTestPointFromNative(const gfx::Point& point) {
  return EmitHitTestPointEventAndReturnValue("hit-test-point", point);
}

std::optional<gfx::Size> View::OnCalculatePreferredSizeFromNative(
    const views::SizeBounds& available_size) {
  v8::Isolate* isolate = this->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> wrapper = GetWrapper();
  if (wrapper.IsEmpty())
    return std::nullopt;

  const double width = available_size.width().is_bounded()
                           ? available_size.width().value()
                           : std::numeric_limits<double>::infinity();
  const double height = available_size.height().is_bounded()
                            ? available_size.height().value()
                            : std::numeric_limits<double>::infinity();
  v8::Local<v8::Value> available_size_value = gin::DataObjectBuilder(isolate)
                                                  .Set("width", width)
                                                  .Set("height", height)
                                                  .Build();
  v8::Local<v8::Value> result = gin_helper::CustomEmit(
      isolate, wrapper, "calculatePreferredSize", available_size_value);

  gfx::Size size;
  if (!gin::ConvertFromV8(isolate, result, &size))
    return std::nullopt;
  return size;
}

void View::OnLayoutFromNative() {
  if (!view_)
    return;

  EmitUnifiedEventObject(this, "layout", std::nullopt,
                         [this](gin_helper::Dictionary* event_dict) {
                           event_dict->Set("width", view_->width());
                           event_dict->Set("height", view_->height());
                         });
}

void View::OnViewBoundsChanged(views::View* observed_view) {
  ApplyBorderRadius();
  UpdateShadowLayer();
  Emit("bounds-changed");
}

void View::OnViewIsDeleting(views::View* observed_view) {
  DCHECK_EQ(observed_view, view_);
  view_ = nullptr;
}

void View::OnChildViewRemoved(views::View* observed_view, views::View* child) {
  std::erase_if(child_views_, [child](const ChildPair& child_view) {
    return child_view.first == child;
  });
}

// static
gin_helper::WrappableBase* View::New(gin::Arguments* args) {
  View* view = new View();
  view->InitWithArgs(args);
  return view;
}

// static
v8::Local<v8::Function> View::GetConstructor(v8::Isolate* isolate) {
  static base::NoDestructor<v8::Global<v8::Function>> constructor;
  if (constructor.get()->IsEmpty()) {
    constructor->Reset(isolate, gin_helper::CreateConstructor<View>(
                                    isolate, base::BindRepeating(&View::New)));
  }
  return v8::Local<v8::Function>::New(isolate, *constructor.get());
}

// static
gin_helper::Handle<View> View::Create(v8::Isolate* isolate) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> obj;
  if (GetConstructor(isolate)->NewInstance(context, 0, nullptr).ToLocal(&obj)) {
    gin_helper::Handle<View> view;
    if (gin::ConvertFromV8(isolate, obj, &view))
      return view;
  }
  return {};
}

// static
void View::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "View"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("addChildView", &View::AddChildViewAt)
      .SetMethod("removeChildView", &View::RemoveChildView)
      .SetProperty("children", &View::GetChildren)
      .SetMethod("setBounds", &View::SetBounds)
      .SetMethod("getBounds", &View::GetBounds)
      .SetMethod("getLocalBounds", &View::GetLocalBounds)
      .SetMethod("setBackgroundColor", &View::SetBackgroundColor)
      .SetMethod("setBorderRadius", &View::SetBorderRadius)
      .SetMethod("setMasksToBounds", &View::SetMasksToBounds)
      .SetMethod("setBorder", &View::SetBorder)
      .SetMethod("setBackgroundBlur", &View::SetBackgroundBlur)
      .SetMethod("setLayout", &View::SetLayout)
      .SetMethod("invalidateLayout", &View::InvalidateLayout)
      .SetMethod("schedulePaint", &View::SchedulePaint)
      .SetMethod("preferredSizeChanged", &View::PreferredSizeChanged)
      .SetMethod("setTranslation", &View::SetTranslation)
      .SetMethod("setScale", &View::SetScale)
      .SetMethod("setRotation", &View::SetRotation)
      .SetMethod("resetTransform", &View::ResetTransform)
      .SetMethod("setShadow", &View::SetShadow)
      .SetMethod("clearShadow", &View::ClearShadow)
      .SetMethod("setLayerOpacity", &View::SetLayerOpacity)
      .SetMethod("setVisible", &View::SetVisible)
      .SetMethod("getVisible", &View::GetVisible)
      .SetMethod("getPreferredSize", &View::GetPreferredSize)
      .SetMethod("setPreferredSize", &View::SetPreferredSize)
      .SetMethod("sizeToContents", &View::SizeToContents);
}

}  // namespace electron::api

namespace {

using electron::api::View;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("View", View::GetConstructor(isolate));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_view, Initialize)
