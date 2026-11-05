// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license.
#ifndef ELECTRON_SHELL_BROWSER_UNOWNED_WEB_CONTENTS_BRIDGE_H_
#define ELECTRON_SHELL_BROWSER_UNOWNED_WEB_CONTENTS_BRIDGE_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/web_contents_observer.h"

namespace electron {

// An embedding relationship only. Never takes ownership of either contents.
class UnownedWebContentsBridge : public content::WebContentsObserver {
 public:
  // Runs after every detach with the outer contents to notify, which is null
  // when the outer contents is being destroyed.
  using DetachedCallback =
      base::RepeatingCallback<void(content::WebContents* outer, std::string)>;

  UnownedWebContentsBridge(content::WebContents* inner,
                           DetachedCallback detached);
  ~UnownedWebContentsBridge() override;

  bool IsAttached() const;
  const std::string& attachment_id() const { return attachment_id_; }
  std::string Attach(content::RenderFrameHost* owner,
                     const std::string& frame_token,
                     std::string* error);
  // Detaches only the given attachment when `attachment_id` is not empty.
  bool Detach(const std::string& attachment_id = {});

 private:
  void UnownedInnerWebContentsDetached(content::WebContents* inner) override;
  void RenderFrameHostStateChanged(
      content::RenderFrameHost* frame,
      content::RenderFrameHost::LifecycleState old_state,
      content::RenderFrameHost::LifecycleState new_state) override;
  void PrimaryMainFrameRenderProcessGone(
      base::TerminationStatus status) override;
  void WebContentsDestroyed() override;
  void CompleteDetach(content::WebContents* outer);

  base::WeakPtr<content::WebContents> inner_;
  base::WeakPtr<content::WebContents> outer_;
  content::GlobalRenderFrameHostId owner_id_;
  DetachedCallback detached_;
  std::string attachment_id_;
  base::WeakPtrFactory<UnownedWebContentsBridge> weak_factory_{this};
};

}  // namespace electron
#endif
