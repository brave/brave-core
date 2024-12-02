/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_shields/cookie_list_opt_in_bubble_host.h"

#include <memory>
#include <optional>

#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ui/webui/brave_shields/cookie_list_opt_in_ui.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace brave_shields {

namespace {

static bool g_allow_in_background_for_testing_ = false;

views::View* GetAnchorView(Browser* browser) {
  DCHECK(browser);
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  DCHECK(browser_view);
  auto* location_bar_view = browser_view->GetLocationBarView();
  DCHECK(location_bar_view);
  return location_bar_view;
}

bool ShouldEventuallyShowBubble() {
  if (!base::FeatureList::IsEnabled(features::kBraveAdblockCookieListOptIn)) {
    return false;
  }

  if (first_run::IsChromeFirstRun()) {
    return false;
  }

  auto* local_state = g_browser_process->local_state();
  if (local_state->GetBoolean(prefs::kAdBlockCookieListOptInShown)) {
    return false;
  }

  base::UmaHistogramExactLinear(kCookieListPromptHistogram, 0, 4);

  auto* component_service_manager =
      g_brave_browser_process->ad_block_service()->component_service_manager();
  DCHECK(component_service_manager);
  if (component_service_manager->IsFilterListEnabled(kCookieListUuid)) {
    return false;
  }

  return true;
}

void ShowBubbleOnSessionRestore(base::WeakPtr<Browser> browser, Profile*, int) {
  if (!browser) {
    return;
  }

  auto* bubble_host = CookieListOptInBubbleHost::FromBrowser(browser.get());
  if (!bubble_host) {
    return;
  }

  bubble_host->ShowBubble();
}

class BubbleManager : public WebUIBubbleManagerImpl<CookieListOptInUI> {
 public:
  BubbleManager(views::View* anchor_view,
                BrowserWindowInterface* browser_window_interface)
      : WebUIBubbleManagerImpl<CookieListOptInUI>(
            anchor_view,
            browser_window_interface,
            GURL(kCookieListOptInURL),
            IDS_BRAVE_SHIELDS,
            /*force_load_on_create=*/false) {}

  ~BubbleManager() override = default;

  // WebUIBubbleManagerImpl<CookieListOptInUI>:
  base::WeakPtr<WebUIBubbleDialogView> CreateWebUIBubbleDialog(
      const std::optional<gfx::Rect>& anchor,
      views::BubbleBorder::Arrow arrow) override {
    auto dialog_view =
        WebUIBubbleManagerImpl<CookieListOptInUI>::CreateWebUIBubbleDialog(
            anchor, arrow);
    DCHECK(dialog_view);
    dialog_view->set_close_on_deactivate(false);
    return dialog_view;
  }
};

}  // namespace

CookieListOptInBubbleHost::CookieListOptInBubbleHost(Browser* browser)
    : BrowserUserData<CookieListOptInBubbleHost>(*browser) {
  GetBrowser().tab_strip_model()->AddObserver(this);
}

CookieListOptInBubbleHost::~CookieListOptInBubbleHost() = default;

void CookieListOptInBubbleHost::MaybeCreateForBrowser(Browser* browser) {
  if (browser->type() == Browser::TYPE_NORMAL && ShouldEventuallyShowBubble()) {
    CreateForBrowser(browser);
  }
}

void CookieListOptInBubbleHost::ShowBubble() {
  // Clear any active session restore callback.
  session_restored_subscription_ = {};

  if (bubble_manager_ && bubble_manager_->GetBubbleWidget()) {
    return;
  }

  if (!ShouldEventuallyShowBubble()) {
    // If we no longer need to show the opt-in bubble clean up this instance. At
    // this point, the `this` pointer is no longer valid.
    RemoveFromBrowser(&GetBrowser());
    return;
  }

  // Do not show the bubble if this is not the currently active browser window.
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(&GetBrowser());
  if (!browser_view || !browser_view->IsActive()) {
    if (!g_allow_in_background_for_testing_) {
      return;
    }
  }

  // Do not show the bubble if the filter list is not yet available, likely
  // because the filter list component has not yet been donwloaded.
  auto* component_service_manager =
      g_brave_browser_process->ad_block_service()->component_service_manager();
  DCHECK(component_service_manager);
  if (!component_service_manager->IsFilterListAvailable(kCookieListUuid)) {
    return;
  }

  // Ensure that the opt-in bubble will not be shown again.
  g_browser_process->local_state()->SetBoolean(
      prefs::kAdBlockCookieListOptInShown, true);

  if (!bubble_manager_) {
    bubble_manager_ = std::make_unique<BubbleManager>(
        GetAnchorView(&GetBrowser()), &GetBrowser());
  }

  if (!bubble_manager_->GetBubbleWidget()) {
    bubble_manager_->ShowBubble();
  }
}

content::WebContents*
CookieListOptInBubbleHost::GetBubbleWebContentsForTesting() {
  if (!bubble_manager_ || !bubble_manager_->GetBubbleWidget()) {
    return nullptr;
  }

  auto bubble_view = bubble_manager_->bubble_view_for_testing();
  if (!bubble_view) {
    return nullptr;
  }

  auto* contents_wrapper = bubble_view->get_contents_wrapper_for_testing();
  return contents_wrapper ? contents_wrapper->web_contents() : nullptr;
}

void CookieListOptInBubbleHost::AllowBubbleInBackgroundForTesting() {
  g_allow_in_background_for_testing_ = true;
}

void CookieListOptInBubbleHost::TabChangedAt(content::WebContents* web_contents,
                                             int index,
                                             TabChangeType change_type) {
  // Exit if this is not a "loaded" event for the active tab in this tab strip.
  if (GetBrowser().tab_strip_model()->active_index() != index ||
      change_type != TabChangeType::kLoadingOnly || web_contents->IsLoading()) {
    return;
  }

  // Exit if we're already waiting for session restore to complete.
  if (session_restored_subscription_) {
    return;
  }

  // If a session is in the middle of restoring, attach a callback to show the
  // bubble once session restore has completed.
  if (SessionRestore::IsRestoring(GetBrowser().profile())) {
    session_restored_subscription_ =
        SessionRestore::RegisterOnSessionRestoredCallback(base::BindRepeating(
            ShowBubbleOnSessionRestore, GetBrowser().AsWeakPtr()));
    return;
  }

  ShowBubble();
}

BROWSER_USER_DATA_KEY_IMPL(CookieListOptInBubbleHost);

}  // namespace brave_shields
