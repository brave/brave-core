/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_

#include <string>

#include "base/callback_forward.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}

namespace brave_talk {

// Service for managing requests to |brave.beginAdvertiseShareDisplayMedia|. Note:
// only one frame can be actively requesting at any time (a subsequent request will replace it).
class BraveTalkService : public KeyedService, content::WebContentsObserver {
 public:
  BraveTalkService();
  BraveTalkService(const BraveTalkService&) = delete;
  BraveTalkService& operator=(const BraveTalkService&) = delete;
  ~BraveTalkService() override;

  // Requests a DeviceID to let a tab be shared with a specific frame in |contents|.
  void GetDeviceID(content::WebContents* contents,
                   int owning_process_id,
                   int owning_frame_id,
                   base::OnceCallback<void(const std::string&)> callback);

  // Shares a tab with whichever GetDeviceID request was most recent.
  void ShareTab(content::WebContents* target_contents);

  // Indicates whether there is a GetDeviceID request pending.
  bool is_requesting_tab() { return !on_received_device_id_.is_null(); }

  // content::WebContentsObserver:
  void DidStartNavigation(content::NavigationHandle* handle) override;

  // Testing:
  // Injects a callback for testing, which is called when a DeviceID has been requested. Used so tests
  // know when to continue.
  void OnGetDeviceIDRequestedForTesting(
      base::OnceCallback<void()> callback_for_testing);

 private:
  void StartObserving(content::WebContents* contents);
  void StopObserving();

  int owning_render_frame_id_;
  int owning_render_process_id_;
  base::OnceCallback<void(const std::string&)> on_received_device_id_;

  // Testing:
  base::OnceCallback<void()> on_get_device_id_requested_for_testing_;
};
}  // namespace brave_talk

#endif  // BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_
