/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_CAPTURE_REGISTRY_H_
#define BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_CAPTURE_REGISTRY_H_

#include <memory>
#include <vector>
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "src/chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"


namespace brave_talk {

class BraveTalkTabCaptureRegistry
    : public extensions::BrowserContextKeyedAPI,
      public MediaCaptureDevicesDispatcher::Observer {
 public:
  BraveTalkTabCaptureRegistry(const BraveTalkTabCaptureRegistry&) = delete;
  BraveTalkTabCaptureRegistry& operator=(const BraveTalkTabCaptureRegistry&) =
      delete;

  static BraveTalkTabCaptureRegistry* Get(content::BrowserContext* context);

  // Used by BrowserContextKeyedAPI.
  static extensions::BrowserContextKeyedAPIFactory<BraveTalkTabCaptureRegistry>*
  GetFactoryInstance();

  std::string AddRequest(content::WebContents* target_contents,
                         content::DesktopMediaID source,
                         content::WebContents* caller_contents);

  bool VerifyRequest(int target_render_process_id, int target_render_frame_id);

 private:
  friend class extensions::BrowserContextKeyedAPIFactory<
      BraveTalkTabCaptureRegistry>;
  class LiveRequest;

  explicit BraveTalkTabCaptureRegistry(content::BrowserContext* context);
  ~BraveTalkTabCaptureRegistry() override;

  LiveRequest* FindRequest(const content::WebContents* target_contents) const;
  LiveRequest* FindRequest(int target_render_process_id,
                           int target_render_frame_id) const;
  void KillRequest(LiveRequest* request);

  // Used by BrowserContextKeyedAPI
  static const char* service_name() { return "BraveTalkTabCaptureRegistry"; }

  const raw_ptr<content::BrowserContext> browser_context_;
  std::vector<std::unique_ptr<LiveRequest>> requests_;
};

}  // namespace brave_talk

#endif  // BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_CAPTURE_REGISTRY_H_
