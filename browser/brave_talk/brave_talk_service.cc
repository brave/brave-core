#include "brave/browser/brave_talk/brave_talk_service.h"

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/share_tab_button.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace brave_talk {

BraveTalkService::BraveTalkService() = default;
BraveTalkService::~BraveTalkService() {
  // TODO: Remove self from all observers.
  StopObserving(observing_.get());
  Shutdown();
}

void BraveTalkService::StartObserving(content::WebContents* contents) {
  if (observing_) {
    StopObserving(observing_.get());
  }

  LOG(ERROR) << "Started watching: " << contents;
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
  LOG(ERROR) << "Stopped watching: " << contents;

  auto* button = share_tab_button();
  if (!button)
    return;
  share_tab_button()->SetVisible(false);

  observing_ = nullptr;
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

}  // namespace brave_talk