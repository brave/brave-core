/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/public/pinned_tab_reset_controller.h"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/map_util.h"
#include "base/feature_list.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/types/to_address.h"
#include "brave/browser/sessions/brave_session_keys.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/base/page_transition_types.h"
#include "ui/events/event.h"
#include "url/url_constants.h"

namespace {

bool IsUsablePinnedUrl(const GURL& url) {
  return url.is_valid() && !url.is_empty() && !url.IsAboutBlank() &&
         !url.SchemeIs(url::kJavaScriptScheme);
}

void PersistPinnedUrl(content::WebContents* contents,
                      const std::string& value) {
  auto* helper = sessions::SessionTabHelper::FromWebContents(contents);
  if (!helper || !helper->window_id().is_valid()) {
    return;
  }
  auto* session_service = SessionServiceFactory::GetForProfileIfExisting(
      Profile::FromBrowserContext(contents->GetBrowserContext()));
  if (!session_service) {
    return;
  }
  session_service->AddTabExtraData(helper->window_id(), helper->session_id(),
                                   kBravePinnedTabUrlKey, value);
}

// Marks contents that must never get a pinned URL, as they only stand in for
// other contents.
class PinnedTabResetExcludedData
    : public content::WebContentsUserData<PinnedTabResetExcludedData> {
 public:
  PinnedTabResetExcludedData(const PinnedTabResetExcludedData&) = delete;
  PinnedTabResetExcludedData& operator=(const PinnedTabResetExcludedData&) =
      delete;
  ~PinnedTabResetExcludedData() override = default;

  content::WebContents* stands_in_for() const { return stands_in_for_.get(); }

 private:
  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  PinnedTabResetExcludedData(content::WebContents* contents,
                             content::WebContents* stands_in_for)
      : content::WebContentsUserData<PinnedTabResetExcludedData>(*contents),
        stands_in_for_(stands_in_for ? stands_in_for->GetWeakPtr() : nullptr) {}

  base::WeakPtr<content::WebContents> stands_in_for_;
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(PinnedTabResetExcludedData);

bool IsExcluded(const content::WebContents* contents) {
  return !!PinnedTabResetExcludedData::FromWebContents(contents);
}

// Returns the contents that hold the pinned URL for `contents`: itself, or for
// a placeholder, the contents it stands in for. Null when there's none.
content::WebContents* GetContentsWithPinnedUrl(content::WebContents* contents) {
  if (const auto* excluded =
          PinnedTabResetExcludedData::FromWebContents(contents)) {
    contents = excluded->stands_in_for();
  }
  return contents && pinned_tab_reset::GetPinnedUrl(contents) ? contents
                                                              : nullptr;
}

// Holds the URL a pinned tab was pinned with. Attached to the WebContents so it
// follows the tab when it moves between windows.
class PinnedTabUrlData : public content::WebContentsUserData<PinnedTabUrlData>,
                         public content::WebContentsObserver {
 public:
  PinnedTabUrlData(const PinnedTabUrlData&) = delete;
  PinnedTabUrlData& operator=(const PinnedTabUrlData&) = delete;
  ~PinnedTabUrlData() override = default;

  static void Set(content::WebContents* contents, const GURL& url) {
    if (IsExcluded(contents)) {
      return;
    }
    PinnedTabUrlData::CreateForWebContents(contents);
    auto* data = PinnedTabUrlData::FromWebContents(contents);
    data->url_ = url;
    data->awaiting_first_commit_ = false;
    PersistPinnedUrl(contents, url.spec());
  }

  // For a tab that is pinned before it has a URL, e.g. one opened on startup:
  // the first committed URL is the one it was pinned with.
  static void AwaitFirstCommit(content::WebContents* contents) {
    if (IsExcluded(contents)) {
      return;
    }
    PinnedTabUrlData::CreateForWebContents(contents);
    PinnedTabUrlData::FromWebContents(contents)->awaiting_first_commit_ = true;
  }

  static void Clear(content::WebContents* contents) {
    if (!PinnedTabUrlData::FromWebContents(contents)) {
      return;
    }
    contents->SetUserData(PinnedTabUrlData::UserDataKey(), {});
    PersistPinnedUrl(contents, std::string());
  }

  const GURL& url() const { return url_; }

 private:
  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  explicit PinnedTabUrlData(content::WebContents* contents)
      : content::WebContentsUserData<PinnedTabUrlData>(*contents),
        content::WebContentsObserver(contents) {}

  // content::WebContentsObserver:
  void DidFinishNavigation(content::NavigationHandle* handle) override {
    if (!awaiting_first_commit_ || !handle->IsInPrimaryMainFrame() ||
        !handle->HasCommitted() || handle->IsErrorPage() ||
        handle->IsSameDocument() || !IsUsablePinnedUrl(handle->GetURL())) {
      return;
    }
    Set(web_contents(), handle->GetURL());
  }

  GURL url_;
  bool awaiting_first_commit_ = false;
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(PinnedTabUrlData);

// Returns the index of the most recently used unpinned tab that isn't
// selected, or nullopt.
std::optional<int> FindMostRecentUnpinnedTab(TabStripModel* model) {
  std::optional<int> result;
  base::TimeTicks result_time;
  for (int i = model->IndexOfFirstNonPinnedTab(); i < model->count(); ++i) {
    if (model->selection_model().IsSelected(model->GetTabAtIndex(i))) {
      continue;
    }
    const base::TimeTicks time =
        model->GetWebContentsAt(i)->GetLastActiveTimeTicks();
    if (!result || time > result_time) {
      result = i;
      result_time = time;
    }
  }
  return result;
}

}  // namespace

DEFINE_USER_DATA(PinnedTabResetController);

PinnedTabResetController::PinnedTabResetController(
    BrowserWindowInterface& browser)
    : browser_(browser),
      scoped_unowned_user_data_(browser.GetUnownedUserDataHost(), *this) {
  browser.GetTabStripModel()->AddObserver(this);
}

PinnedTabResetController::~PinnedTabResetController() = default;

// static
PinnedTabResetController* PinnedTabResetController::From(
    BrowserWindowInterface* browser) {
  return browser ? Get(browser->GetUnownedUserDataHost()) : nullptr;
}

bool PinnedTabResetController::HandleCloseTabShortcut() {
  TabStripModel* model = browser_->GetTabStripModel();

  // Like chrome::CloseTab(), a selection of just one split only acts on the
  // active half.
  std::vector<tabs::TabInterface*> selected_tabs;
  tabs::TabInterface* active_tab = model->GetActiveTab();
  if (active_tab && active_tab->IsSplit() &&
      model->selection_model().size() == 2) {
    selected_tabs.push_back(active_tab);
  } else {
    for (const int index :
         model->selection_model().GetListSelectionModel().selected_indices()) {
      selected_tabs.push_back(model->GetTabAtIndex(index));
    }
  }

  // Pinned tabs without a pinned URL, e.g. not loaded yet, close as usual. A
  // shared pinned tab placeholder resets the tab it stands for instead.
  std::vector<content::WebContents*> contents_to_reset;
  std::vector<content::WebContents*> contents_to_close;
  for (tabs::TabInterface* tab : selected_tabs) {
    if (sidebar::IsWebPanelTab(tab)) {
      return false;
    }
    content::WebContents* contents = tab->GetContents();
    if (tab->IsPinned()) {
      if (content::WebContents* to_reset = GetContentsWithPinnedUrl(contents)) {
        contents_to_reset.push_back(to_reset);
        continue;
      }
      // Closing a placeholder would close the tab in every window.
      if (IsExcluded(contents)) {
        continue;
      }
    }
    contents_to_close.push_back(contents);
  }
  if (contents_to_reset.empty()) {
    return false;
  }

  // Leave the pinned tabs first, as closing the others would otherwise pick
  // the neighbouring tab, which may be pinned.
  if (const std::optional<int> target = FindMostRecentUnpinnedTab(model)) {
    model->ActivateTabAt(*target,
                         TabStripUserGestureDetails(
                             TabStripUserGestureDetails::GestureType::kOther));
  } else {
    chrome::AddTabAt(base::to_address(browser_),
                     chrome::GetNewTabURL(base::to_address(browser_)),
                     /*index=*/-1, /*foreground=*/true,
                     /*group=*/std::nullopt,
                     /*pinned=*/false);
  }

  for (content::WebContents* contents : contents_to_reset) {
    ResetToPinnedUrl(contents);
  }
  for (content::WebContents* contents : contents_to_close) {
    model->CloseWebContents(contents,
                            TabCloseTypes::CLOSE_USER_GESTURE |
                                TabCloseTypes::CLOSE_CREATE_HISTORICAL_TAB);
  }
  return true;
}

void PinnedTabResetController::ResetPinnedTab(tabs::TabInterface* tab) {
  if (!tab->IsPinned() || !tab->GetContents()) {
    return;
  }
  if (content::WebContents* contents =
          GetContentsWithPinnedUrl(tab->GetContents())) {
    ResetToPinnedUrl(contents);
  }
}

// static
void PinnedTabResetController::ResetToPinnedUrl(
    content::WebContents* contents) {
  const std::optional<GURL> pinned_url =
      pinned_tab_reset::GetPinnedUrl(contents);
  if (!pinned_url) {
    return;
  }

  content::NavigationController& controller = contents->GetController();
  const content::NavigationEntry* entry = controller.GetLastCommittedEntry();
  if (entry && entry->GetURL() == *pinned_url &&
      !controller.GetPendingEntry()) {
    controller.PruneAllButLastCommitted();
    return;
  }

  content::NavigationController::LoadURLParams params(*pinned_url);
  params.transition_type = ui::PAGE_TRANSITION_AUTO_TOPLEVEL;
  params.should_clear_history_list = true;
  controller.LoadURLWithParams(params);
}

void PinnedTabResetController::RestorePinnedUrl(content::WebContents* contents,
                                                const GURL& url) {
  TabStripModel* model = browser_->GetTabStripModel();
  const int index = model->GetIndexOfWebContents(contents);
  if (index == TabStripModel::kNoTab || !model->IsTabPinned(index) ||
      !IsUsablePinnedUrl(url) ||
      sidebar::IsWebPanelTab(base::to_address(browser_), contents)) {
    return;
  }
  PinnedTabUrlData::Set(contents, url);
}

void PinnedTabResetController::PopulateExtraData(
    int index,
    std::map<std::string, std::string>& extra_data) const {
  if (const auto url = pinned_tab_reset::GetPinnedUrl(
          browser_->GetTabStripModel()->GetWebContentsAt(index))) {
    extra_data[kBravePinnedTabUrlKey] = url->spec();
  }
}

void PinnedTabResetController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (change.type() == TabStripModelChange::kInserted) {
    for (const auto& inserted : change.GetInsert()->contents) {
      // Tabs moved from another window already carry their URL.
      if (inserted.tab->IsPinned() &&
          !PinnedTabUrlData::FromWebContents(inserted.contents)) {
        RecordPinnedUrl(inserted.contents);
      }
    }
  } else if (change.type() == TabStripModelChange::kReplaced) {
    // E.g. a discarded tab gets new contents.
    const auto* replace = change.GetReplace();
    // Unpinning a shared pinned tab placeholder replaces it with the real
    // contents, which must forget their pinned URL.
    if (!replace->tab->IsPinned()) {
      PinnedTabUrlData::Clear(replace->new_contents);
      return;
    }
    // Never overwrites the URL the new contents already has: shared pinned
    // tabs swap the real contents with a placeholder this way.
    if (PinnedTabUrlData::FromWebContents(replace->new_contents)) {
      return;
    }
    if (const auto url =
            pinned_tab_reset::GetPinnedUrl(replace->old_contents)) {
      PinnedTabUrlData::Set(replace->new_contents, *url);
    }
  }
}

void PinnedTabResetController::OnTabPinnedStateChanged(tabs::TabInterface* tab,
                                                       int index) {
  content::WebContents* contents = tab->GetContents();
  if (!contents) {
    return;
  }
  if (tab->IsPinned()) {
    RecordPinnedUrl(contents);
  } else {
    PinnedTabUrlData::Clear(contents);
  }
}

void PinnedTabResetController::RecordPinnedUrl(content::WebContents* contents) {
  if (sidebar::IsWebPanelTab(base::to_address(browser_), contents)) {
    return;
  }
  // A pending URL may never commit, so a tab without a committed page waits
  // for its first one.
  const GURL& url = contents->GetLastCommittedURL();
  if (IsUsablePinnedUrl(url)) {
    PinnedTabUrlData::Set(contents, url);
  } else {
    PinnedTabUrlData::AwaitFirstCommit(contents);
  }
}

namespace pinned_tab_reset {

std::optional<GURL> GetPinnedUrl(const content::WebContents* contents) {
  if (!base::FeatureList::IsEnabled(tabs::kBravePinnedTabReset)) {
    return std::nullopt;
  }
  const auto* data = PinnedTabUrlData::FromWebContents(contents);
  if (!data || IsExcluded(contents) || !IsUsablePinnedUrl(data->url())) {
    return std::nullopt;
  }
  return data->url();
}

void ExcludeFromPinnedTabReset(content::WebContents* contents,
                               content::WebContents* stands_in_for) {
  PinnedTabResetExcludedData::CreateForWebContents(contents, stands_in_for);
}

void MaybeResetOnMousePressed(tabs::TabInterface* tab,
                              const ui::MouseEvent& event) {
  if (!tab || !tab->IsPinned() || !event.IsOnlyLeftMouseButton() ||
      event.GetClickCount() != 2 || event.IsShiftDown() ||
      event.IsControlDown() || event.IsAltDown() || event.IsCommandDown()) {
    return;
  }
  if (auto* controller =
          PinnedTabResetController::From(tab->GetBrowserWindowInterface())) {
    controller->ResetPinnedTab(tab);
  }
}

void MaybePopulateExtraData(BrowserWindowInterface& browser,
                            int index,
                            std::map<std::string, std::string>& extra_data) {
  if (auto* controller = PinnedTabResetController::From(&browser)) {
    controller->PopulateExtraData(index, extra_data);
  }
}

void MaybeRestoreFromExtraData(
    BrowserWindowInterface* browser,
    content::WebContents* restored_contents,
    const std::map<std::string, std::string>& extra_data) {
  auto* controller = PinnedTabResetController::From(browser);
  if (!controller || !restored_contents) {
    return;
  }
  if (const std::string* url =
          base::FindOrNull(extra_data, kBravePinnedTabUrlKey)) {
    controller->RestorePinnedUrl(restored_contents, GURL(*url));
  }
}

}  // namespace pinned_tab_reset
