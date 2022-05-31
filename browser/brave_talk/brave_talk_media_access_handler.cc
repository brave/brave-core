/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_talk/brave_talk_media_access_handler.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_talk/brave_talk_service.h"
#include "brave/browser/brave_talk/brave_talk_tab_capture_registry.h"
#include "chrome/browser/media/webrtc/capture_policy_utils.h"
#include "chrome/browser/media/webrtc/desktop_capture_devices_util.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "chrome/browser/media/webrtc/media_stream_capture_indicator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tab_sharing/tab_sharing_ui.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/desktop_media_id.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_media_capture_id.h"
#include "src/chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom-shared.h"

namespace brave_talk {

BraveTalkMediaAccessHandler::BraveTalkMediaAccessHandler() = default;
BraveTalkMediaAccessHandler::~BraveTalkMediaAccessHandler() = default;

bool BraveTalkMediaAccessHandler::SupportsStreamType(
    content::WebContents* web_contents,
    const blink::mojom::MediaStreamType type,
    const extensions::Extension* extension) {
  if (!web_contents)
    return false;

  auto* registry = BraveTalkTabCaptureRegistry::GetInstance();
  if (!registry || !registry->VerifyRequest(
                       web_contents->GetMainFrame()->GetProcess()->GetID(),
                       web_contents->GetMainFrame()->GetRoutingID())) {
    return false;
  }

  return type == blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE ||
         type == blink::mojom::MediaStreamType::GUM_TAB_VIDEO_CAPTURE;
}

bool BraveTalkMediaAccessHandler::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    blink::mojom::MediaStreamType type,
    const extensions::Extension* extension) {
  return false;
}

void BraveTalkMediaAccessHandler::HandleRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback,
    const extensions::Extension* extension) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto* registry = BraveTalkTabCaptureRegistry::GetInstance();
  if (!registry) {
    NOTREACHED();
    std::move(callback).Run(
        blink::MediaStreamDevices(),
        blink::mojom::MediaStreamRequestResult::INVALID_STATE, /*ui=*/nullptr);
    return;
  }

  AllowedScreenCaptureLevel capture_level =
      capture_policy::GetAllowedCaptureLevel(request.security_origin,
                                             web_contents);
  DesktopMediaList::WebContentsFilter can_show_web_contents =
      capture_policy::GetIncludableWebContentsFilter(request.security_origin,
                                                     capture_level);

  if (!can_show_web_contents.Run(web_contents)) {
    std::move(callback).Run(
        blink::MediaStreamDevices(),
        blink::mojom::MediaStreamRequestResult::PERMISSION_DENIED,
        /*ui=*/nullptr);
    return;
  }

  if (!registry->VerifyRequest(
          web_contents->GetMainFrame()->GetProcess()->GetID(),
          web_contents->GetMainFrame()->GetRoutingID())) {
    std::move(callback).Run(
        blink::MediaStreamDevices(),
        blink::mojom::MediaStreamRequestResult::INVALID_STATE,
        /*ui=*/nullptr);
    return;
  }

  content::DesktopMediaID media_id(
      content::DesktopMediaID::TYPE_WEB_CONTENTS,
      content::DesktopMediaID::kNullId,
      content::WebContentsMediaCaptureId(
          web_contents->GetMainFrame()->GetProcess()->GetID(),
          web_contents->GetMainFrame()->GetRoutingID()));
  AcceptRequest(web_contents, request, media_id, std::move(callback));
}

void BraveTalkMediaAccessHandler::AcceptRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::DesktopMediaID& media_id,
    content::MediaResponseCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(web_contents);

  blink::MediaStreamDevices devices;

  std::u16string application_title =
      base::UTF8ToUTF16(BraveTalkService::GetInstance()
                            ->web_contents()
                            ->GetMainFrame()
                            ->GetLastCommittedOrigin()
                            .Serialize());
  std::unique_ptr<content::MediaStreamUI> ui =
      GetDevicesForDesktopCapture(request, BraveTalkService::GetInstance()->web_contents(), media_id, request.audio_type == blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE, true,
                                  true, application_title, &devices);
  DCHECK(!devices.empty());

  std::move(callback).Run(devices, blink::mojom::MediaStreamRequestResult::OK,
                          std::move(ui));
}

}  // namespace brave_talk
