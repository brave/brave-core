#include "brave/browser/brave_talk/brave_talk_service.h"
#include <algorithm>

#include "base/callback.h"
#include "brave/browser/brave_talk/brave_talk_tab_capture_registry.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/share_tab_button.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content\public\browser\render_process_host.h"

namespace brave_talk {

BraveTalkService::BraveTalkService() = default;
BraveTalkService::~BraveTalkService() {
  // TODO: Remove self from all observers.
  StopObserving(observing_.get());
  Shutdown();
}

void BraveTalkService::GetDeviceID(
    content::WebContents* contents,
    base::OnceCallback<void(std::string)> callback) {
  if (on_received_device_id_)
    std::move(on_received_device_id_).Run("");

  on_received_device_id_ = std::move(callback);
  StartObserving(contents);
}

void BraveTalkService::StartObserving(content::WebContents* contents) {
  if (observing_)
    StopObserving(observing_.get());

  // TODO: We should be able to observe multiple web contents.
  observing_ = contents->GetWeakPtr();
  Observe(contents);

  auto* button = share_tab_button();
  if (!button)
    return;
  button->SetVisible(true);
  button->UpdateImageAndText();
}

void BraveTalkService::StopObserving(content::WebContents* contents) {
  auto* button = share_tab_button();
  if (!button)
    return;
  share_tab_button()->SetVisible(false);

  observing_ = nullptr;
  target_ = nullptr;
  Observe(nullptr);
}

void BraveTalkService::DidStartNavigation(content::NavigationHandle* handle) {
  if (!handle->IsInMainFrame())
    return;

  // On any navigation of the main frame stop observing the web contents.
  StopObserving(handle->GetWebContents());
}

share_tab_button::ShareTabButton* BraveTalkService::share_tab_button() {
  if (!observing_)
    return nullptr;

  auto* browser = chrome::FindBrowserWithWebContents(observing_.get());
  if (!browser)
    return nullptr;

  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  auto* toolbar = static_cast<BraveToolbarView*>(browser_view->toolbar());
  return toolbar->share_tab_button();
}

void BraveTalkService::ShareTab(content::WebContents* target_contents) {
  target_ = target_contents->GetWeakPtr();
  share_tab_button()->SetVisible(false);

  if (!observing_)
    return;
  auto* registry =
      BraveTalkTabCaptureRegistry::Get(target_contents->GetBrowserContext());

  content::DesktopMediaID media_id(
      content::DesktopMediaID::TYPE_WEB_CONTENTS,
      content::DesktopMediaID::kNullId,
      content::WebContentsMediaCaptureId(
          target_contents->GetMainFrame()->GetProcess()->GetID(),
          target_contents->GetMainFrame()->GetRoutingID()));
  std::string device_id = registry->AddRequest(
      target_contents, observing_->GetURL(), media_id, observing_.get());
  if (on_received_device_id_)
    std::move(on_received_device_id_).Run(device_id);
}

}  // namespace brave_talk