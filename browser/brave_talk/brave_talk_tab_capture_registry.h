/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_CAPTURE_REGISTRY_H_
#define BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_CAPTURE_REGISTRY_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/singleton.h"
#include "content/public/browser/render_frame_host.h"
#include "src/chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"

namespace brave_talk {

class BraveTalkTabCaptureRegistry {
 public:
  ~BraveTalkTabCaptureRegistry();
  BraveTalkTabCaptureRegistry(const BraveTalkTabCaptureRegistry&) = delete;
  BraveTalkTabCaptureRegistry& operator=(const BraveTalkTabCaptureRegistry&) =
      delete;

  static BraveTalkTabCaptureRegistry* GetInstance();

  std::string AddRequest(content::WebContents* target_contents,
                         content::RenderFrameHost* owning_frame);

  bool VerifyRequest(int target_render_process_id, int target_render_frame_id);

 private:
  BraveTalkTabCaptureRegistry();
  friend struct base::DefaultSingletonTraits<BraveTalkTabCaptureRegistry>;

  class LiveRequest;

  LiveRequest* FindRequest(int target_render_process_id,
                           int target_render_frame_id) const;
  void KillRequest(LiveRequest* request);

  std::vector<std::unique_ptr<LiveRequest>> requests_;
};

}  // namespace brave_talk

#endif  // BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_CAPTURE_REGISTRY_H_
