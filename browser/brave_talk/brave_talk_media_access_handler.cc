#include "brave/browser/brave_talk/brave_talk_media_access_handler.h"
#include <algorithm>
#include <memory>

#include "brave/browser/brave_talk/brave_talk_service.h"
#include "brave/browser/brave_talk/brave_talk_service_factory.h"
#include "brave/browser/brave_talk/brave_talk_tab_capture_registry.h"
#include "chrome/browser/media/webrtc/media_stream_capture_indicator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/media/webrtc/capture_policy_utils.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_media_capture_id.h"
#include "src/chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"

namespace brave_talk {

namespace {
// Returns an instance of MediaStreamUI to be passed to content layer and stores
// the list of media stream devices for tab capture in |out_devices|.
std::unique_ptr<content::MediaStreamUI> GetMediaStreamUI(
    const content::MediaStreamRequest& request,
    content::WebContents* web_contents,
    blink::MediaStreamDevices* out_devices) {
  content::DesktopMediaID media_id(
      content::DesktopMediaID::TYPE_WEB_CONTENTS,
      content::DesktopMediaID::kNullId,
      content::WebContentsMediaCaptureId(
          web_contents->GetMainFrame()->GetProcess()->GetID(),
          web_contents->GetMainFrame()->GetRoutingID()));
  if (request.audio_type ==
      blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE) {
    out_devices->emplace_back(blink::MediaStreamDevice(
        blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE,
        /*id=*/media_id.ToString(),
        /*name=*/media_id.ToString()));
  }

  if (request.video_type ==
      blink::mojom::MediaStreamType::GUM_TAB_VIDEO_CAPTURE) {
    out_devices->emplace_back(blink::MediaStreamDevice(
        blink::mojom::MediaStreamType::GUM_TAB_VIDEO_CAPTURE,
        /*id=*/media_id.ToString(),
        /*name=*/media_id.ToString()));
  }

  return MediaCaptureDevicesDispatcher::GetInstance()
      ->GetMediaStreamCaptureIndicator()
      ->RegisterMediaStream(web_contents, *out_devices);
}

}  // namespace

BraveTalkMediaAccessHandler::BraveTalkMediaAccessHandler() = default;
BraveTalkMediaAccessHandler::~BraveTalkMediaAccessHandler() = default;

bool BraveTalkMediaAccessHandler::SupportsStreamType(
    content::WebContents* web_contents,
    const blink::mojom::MediaStreamType type,
    const extensions::Extension* extension) {
  if (!web_contents)
    return false;

  auto* registry = brave_talk::BraveTalkTabCaptureRegistry::Get(web_contents->GetBrowserContext());
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

  auto* registry = brave_talk::BraveTalkTabCaptureRegistry::Get(web_contents->GetBrowserContext());
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

  AcceptRequest(web_contents, request, std::move(callback));
}

void BraveTalkMediaAccessHandler::AcceptRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(web_contents);

  blink::MediaStreamDevices devices;
  std::unique_ptr<content::MediaStreamUI> ui =
      GetMediaStreamUI(request, web_contents, &devices);
  DCHECK(!devices.empty());

  std::move(callback).Run(devices, blink::mojom::MediaStreamRequestResult::OK,
                          std::move(ui));
}

}  // namespace brave_talk