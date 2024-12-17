/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/brave_file_select_utils.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/lifetime/browser_close_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/common/webui_url_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/common/url_constants.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"
#include "url/gurl.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#endif

namespace {

bool g_suppress_dialog_for_testing = false;

}  // namespace

// static
void BraveBrowser::SuppressBrowserWindowClosingDialogForTesting(bool suppress) {
  g_suppress_dialog_for_testing = suppress;
}

// static
bool BraveBrowser::ShouldUseBraveWebViewRoundedCorners(Browser* browser) {
  return base::FeatureList::IsEnabled(features::kBraveWebViewRoundedCorners) &&
         browser->is_type_normal();
}

BraveBrowser::BraveBrowser(const CreateParams& params) : Browser(params) {
#if defined(TOOLKIT_VIEWS)
  if (!sidebar::CanUseSidebar(this)) {
    return;
  }
  // Below call order is important.
  // When reaches here, Sidebar UI is setup in BraveBrowserView but
  // not initialized. It's just empty because sidebar controller/model is not
  // ready yet. BraveBrowserView is instantiated by the ctor of Browser.
  // So, initializing sidebar controller/model here and then ask to initialize
  // sidebar UI. After that, UI will be updated for model's change.
  sidebar_controller_ =
      std::make_unique<sidebar::SidebarController>(this, profile());
  sidebar_controller_->SetSidebar(brave_window()->InitSidebar());
#endif
}

BraveBrowser::~BraveBrowser() = default;

void BraveBrowser::ScheduleUIUpdate(content::WebContents* source,
                                    unsigned changed_flags) {
  Browser::ScheduleUIUpdate(source, changed_flags);

#if defined(TOOLKIT_VIEWS)
  if (tab_strip_model_->GetIndexOfWebContents(source) ==
      TabStripModel::kNoTab) {
    return;
  }

  // We need to update sidebar UI only when current active tab state is changed.
  if (changed_flags & content::INVALIDATE_TYPE_URL) {
    if (source == tab_strip_model_->GetActiveWebContents()) {
      // sidebar() can return a nullptr in unit tests.
      if (sidebar_controller_) {
        if (sidebar_controller_->sidebar()) {
          sidebar_controller_->sidebar()->UpdateSidebarItemsState();
        } else {
          CHECK_IS_TEST();
        }
      }
    }
  }
#endif
}

void BraveBrowser::OnTabClosing(content::WebContents* contents) {
  Browser::OnTabClosing(contents);

  if (!AreAllTabsSharedPinnedTabs()) {
    return;
  }

  if (chrome::FindAllTabbedBrowsersWithProfile(profile()).size() > 1) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(
                       [](base::WeakPtr<BraveBrowser> browser) {
                         if (browser) {
                           // We don't want close confirm dialog to show up. In
                           // this case, Shared pinned tabs will be moved to
                           // another window, so we don't have to warn users.
                           browser->confirmed_to_close_ = true;
                           chrome::CloseWindow(browser.get());
                         }
                       },
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void BraveBrowser::TabStripEmpty() {
  if (unload_controller_.is_attempting_to_close_browser() ||
      !is_type_normal() || ignore_enable_closing_last_tab_pref_) {
    Browser::TabStripEmpty();
    return;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&chrome::AddTabAt, this, GetNewTabURL(), -1,
                                true, std::nullopt));
}

void BraveBrowser::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const blink::mojom::FileChooserParams& params) {
#if BUILDFLAG(IS_ANDROID)
  Browser::RunFileChooser(render_frame_host, listener, params);
#else
  auto new_params = params.Clone();
  if (new_params->title.empty()) {
    // Fill title of file chooser with origin of the frame.

    // Note that save mode param is for PPAPI. 'Save As...' or downloading
    // something doesn't reach here. They show 'select file dialog' from
    // DownloadFilePicker::DownloadFilePicker directly.
    // https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/public/mojom/choosers/file_chooser.mojom;l=27;drc=047c7dc4ee1ce908d7fea38ca063fa2f80f92c77
    CHECK(render_frame_host);
    const url::Origin& origin = render_frame_host->GetLastCommittedOrigin();
    new_params->title = brave::GetFileSelectTitle(
        content::WebContents::FromRenderFrameHost(render_frame_host),
        origin, origin,
        params.mode == blink::mojom::FileChooserParams::Mode::kSave
            ? brave::FileSelectTitleType::kSave
            : brave::FileSelectTitleType::kOpen);
  }
  Browser::RunFileChooser(render_frame_host, listener, *new_params);
#endif
}

bool BraveBrowser::ShouldDisplayFavicon(
    content::WebContents* web_contents) const {
  // Override to not show favicon for NTP in tab.

  // Suppress the icon for the new-tab page, even if a navigation to it is
  // not committed yet. Note that we're looking at the visible URL, so
  // navigations from NTP generally don't hit this case and still show an icon.
  GURL url = web_contents->GetVisibleURL();
  if (url.SchemeIs(content::kChromeUIScheme) &&
      url.host_piece() == chrome::kChromeUINewTabHost) {
    return false;
  }

  // Also suppress instant-NTP. This does not use search::IsInstantNTP since
  // it looks at the last-committed entry and we need to show icons for pending
  // navigations away from it.
  if (search::IsInstantNTPURL(url, profile())) {
    return false;
  }

  return Browser::ShouldDisplayFavicon(web_contents);
}

void BraveBrowser::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  Browser::OnTabStripModelChanged(tab_strip_model, change, selection);

  if (!profile()->GetPrefs()->GetBoolean(kEnableClosingLastTab) &&
      change.type() == TabStripModelChange::kRemoved) {
    for (const auto& contents : change.GetRemove()->contents) {
      // If there is no tab after this change for inserting them to
      // another window, this browser should be closed.
      if (contents.remove_reason ==
              TabStripModelChange::RemoveReason::kInsertedIntoOtherTabStrip &&
          tab_strip_model->empty()) {
        // Each removed can only have same reason. so safe to early return here.
        ignore_enable_closing_last_tab_pref_ = true;
        break;
      }
    }
  }

#if defined(TOOLKIT_VIEWS)
  // sidebar() can return a nullptr in unit tests.
  if (!sidebar_controller_ || !sidebar_controller_->sidebar()) {
    return;
  }
  // We need to update sidebar UI whenever active tab is changed or
  // inactive tab is added/removed.
  if (change.type() == TabStripModelChange::Type::kInserted ||
      change.type() == TabStripModelChange::Type::kRemoved ||
      selection.active_tab_changed()) {
    sidebar_controller_->sidebar()->UpdateSidebarItemsState();
  }
#endif
}

void BraveBrowser::FinishWarnBeforeClosing(WarnBeforeClosingResult result) {
  // Clear user's choice because user cancelled window closing by some
  // warning(ex, download is in-progress).
  if (result == WarnBeforeClosingResult::kDoNotClose) {
    confirmed_to_close_ = false;
  }
  Browser::FinishWarnBeforeClosing(result);
}

void BraveBrowser::BeforeUnloadFired(content::WebContents* source,
                                     bool proceed,
                                     bool* proceed_to_fire_unload) {
  // Clear user's choice when user cancelled window closing by beforeunload
  // handler.
  if (!proceed) {
    confirmed_to_close_ = false;
  }
  Browser::BeforeUnloadFired(source, proceed, proceed_to_fire_unload);
}

bool BraveBrowser::TryToCloseWindow(
    bool skip_beforeunload,
    const base::RepeatingCallback<void(bool)>& on_close_confirmed) {
  // Window closing could be asked directly to browser object by this method.
  // For example, when user tries to delete profile, this method is called on
  // all its browser object. After all handlers are done, its all browser window
  // start to close. In this case, we should not ask to users about this
  // closing. So, treats like user confirmed closing. If this try blocked by
  // user, |confirmed_to_close_| is set to false by ResetTryToCloseWindow().
  confirmed_to_close_ = true;
  return Browser::TryToCloseWindow(skip_beforeunload,
                                   std::move(on_close_confirmed));
}

void BraveBrowser::ResetTryToCloseWindow() {
  confirmed_to_close_ = false;
  Browser::ResetTryToCloseWindow();
}

void BraveBrowser::UpdateTargetURL(content::WebContents* source,
                                   const GURL& url) {
  GURL target_url = url;
  if (url.SchemeIs(content::kChromeUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kBraveUIScheme);
    target_url = target_url.ReplaceComponents(replacements);
  }
  Browser::UpdateTargetURL(source, target_url);
}

bool BraveBrowser::ShouldAskForBrowserClosingBeforeHandlers() {
  if (g_suppress_dialog_for_testing) {
    return false;
  }

  // Don't need to ask when application closing is in-progress.
  if (BrowserCloseManager::BrowserClosingStarted()) {
    return false;
  }

  if (confirmed_to_close_) {
    return false;
  }

  PrefService* prefs = profile()->GetPrefs();
  if (!prefs->GetBoolean(kEnableWindowClosingConfirm)) {
    return false;
  }

  // Only launch confirm dialog while closing when browser has multiple tabs.
  return tab_strip_model()->count() > 1;
}

bool BraveBrowser::AreAllTabsSharedPinnedTabs() {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs)) {
    return false;
  }

  if (!is_type_normal()) {
    return false;
  }

  if (!profile()->GetPrefs()->GetBoolean(brave_tabs::kSharedPinnedTab)) {
    return false;
  }

  return tab_strip_model()->count() > 0 &&
         tab_strip_model()->count() ==
             tab_strip_model()->IndexOfFirstNonPinnedTab();
}

BraveBrowserWindow* BraveBrowser::brave_window() {
  return static_cast<BraveBrowserWindow*>(window_);
}
