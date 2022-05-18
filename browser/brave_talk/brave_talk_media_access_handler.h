/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_MEDIA_ACCESS_HANDLER_H_
#define BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_MEDIA_ACCESS_HANDLER_H_

#include <memory>
#include "chrome/browser/media/capture_access_handler_base.h"
#include "content/public/browser/media_stream_request.h"

class MediaStreamUI;

namespace contents {
class WebContents;
}

namespace extensions {
class Extensions;
}
namespace brave_talk {

class BraveTalkTabCaptureRegistry;

class BraveTalkMediaAccessHandler : public CaptureAccessHandlerBase {
 public:
  BraveTalkMediaAccessHandler();

  BraveTalkMediaAccessHandler(const BraveTalkMediaAccessHandler&) = delete;
  BraveTalkMediaAccessHandler& operator=(const BraveTalkMediaAccessHandler&) =
      delete;
  ~BraveTalkMediaAccessHandler() override;

  bool SupportsStreamType(content::WebContents* web_contents,
                          const blink::mojom::MediaStreamType type,
                          const extensions::Extension* extension) override;
  bool CheckMediaAccessPermission(
      content::RenderFrameHost* render_frame_host,
      const GURL& security_origin,
      blink::mojom::MediaStreamType type,
      const extensions::Extension* extension) override;
  void HandleRequest(content::WebContents* web_contents,
                     const content::MediaStreamRequest& request,
                     content::MediaResponseCallback callback,
                     const extensions::Extension* extension) override;

 private:
  void AcceptRequest(content::WebContents* web_contents,
                     const content::MediaStreamRequest& request,
                     content::MediaResponseCallback callback);

  BraveTalkTabCaptureRegistry* GetRegistry(content::WebContents* contents);
};

}  // namespace brave_talk

#endif