/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_talk/brave_talk_tab_capture_registry.h"

#include <memory>

#include "base/lazy_instance.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/desktop_streams_registry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents_observer.h"

namespace brave_talk {

class BraveTalkTabCaptureRegistry::LiveRequest
    : public content::WebContentsObserver {
 public:
  LiveRequest(content::WebContents* target_contents,
              BraveTalkTabCaptureRegistry* registry)
      : content::WebContentsObserver(target_contents),
        registry_(registry),
        render_process_id_(
            target_contents->GetMainFrame()->GetProcess()->GetID()),
        render_frame_id_(target_contents->GetMainFrame()->GetRoutingID()) {
    DCHECK(web_contents());
    DCHECK(registry_);
  }

  LiveRequest(const LiveRequest&) = delete;
  LiveRequest& operator=(const LiveRequest&) = delete;
  ~LiveRequest() override = default;

  bool WasTargettingRenderFrameID(int render_process_id,
                                  int render_frame_id) const {
    return render_process_id_ == render_process_id &&
           render_frame_id_ == render_frame_id;
  }

 protected:
  void WebContentsDestroyed() override {
    registry_->KillRequest(this);  // Deletes |this|.
  }

 private:
  const raw_ptr<BraveTalkTabCaptureRegistry> registry_;
  int render_process_id_;
  int render_frame_id_;
};

BraveTalkTabCaptureRegistry::BraveTalkTabCaptureRegistry(
    content::BrowserContext* context)
    : browser_context_(context) {}

BraveTalkTabCaptureRegistry::~BraveTalkTabCaptureRegistry() = default;

std::string BraveTalkTabCaptureRegistry::AddRequest(
    content::WebContents* target_contents,
    content::RenderFrameHost* owning_frame) {
  content::DesktopMediaID media_id(
      content::DesktopMediaID::TYPE_WEB_CONTENTS,
      content::DesktopMediaID::kNullId,
      content::WebContentsMediaCaptureId(
          target_contents->GetMainFrame()->GetProcess()->GetID(),
          target_contents->GetMainFrame()->GetRoutingID()));

  std::string device_id;
  LiveRequest* const request =
      FindRequest(target_contents->GetMainFrame()->GetProcess()->GetID(),
                  target_contents->GetMainFrame()->GetRoutingID());

  if (request) {
    // Delete the request before creating it's replacement.
    KillRequest(request);
  }

  requests_.push_back(std::make_unique<LiveRequest>(target_contents, this));
  device_id = content::DesktopStreamsRegistry::GetInstance()->RegisterStream(
      owning_frame->GetProcess()->GetID(), owning_frame->GetRoutingID(),
      url::Origin::Create(owning_frame->GetLastCommittedURL()), media_id, "",
      content::kRegistryStreamTypeTab);

  return device_id;
}

BraveTalkTabCaptureRegistry::LiveRequest*
BraveTalkTabCaptureRegistry::FindRequest(int target_render_process_id,
                                         int target_render_frame_id) const {
  for (const auto& request : requests_) {
    if (request->WasTargettingRenderFrameID(target_render_process_id,
                                            target_render_frame_id))
      return request.get();
  }
  return nullptr;
}

void BraveTalkTabCaptureRegistry::KillRequest(LiveRequest* request) {
  for (auto it = requests_.begin(); it != requests_.end(); ++it) {
    if (it->get() == request) {
      requests_.erase(it);
      return;
    }
  }
  NOTREACHED();
}

bool BraveTalkTabCaptureRegistry::VerifyRequest(int target_render_process_id,
                                                int target_render_frame_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  LiveRequest* const request =
      FindRequest(target_render_process_id, target_render_frame_id);
  if (!request) {
    return false;  // Unknown RenderFrameHost ID, or frame has gone away.
  }

  return true;
}

}  // namespace brave_talk
