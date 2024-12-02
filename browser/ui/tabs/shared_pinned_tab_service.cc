/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/notreached.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_dummy_view.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

#define LOCK_REENTRANCE(tab_strip_model)                                  \
  DCHECK(!change_source_model_) << "Already locked";                      \
  base::AutoReset<raw_ptr<TabStripModel>> resetter(&change_source_model_, \
                                                   raw_ptr(tab_strip_model))

namespace {

////////////////////////////////////////////////////////////////////////////////
// DummyContentsData is a WebContentsUserData attached to a dummy web contents
// we create for inactive pinned tabs.
//
class DummyContentsData
    : public content::WebContentsUserData<DummyContentsData> {
 public:
  DummyContentsData(const DummyContentsData&) = delete;
  DummyContentsData& operator=(const DummyContentsData&) = delete;
  ~DummyContentsData() override;

  static void RemoveFromWebContents(content::WebContents* contents);

  void SynchronizeURL();

  content::WebContents* dummy_contents() { return shared_contents_; }
  content::WebContents* shared_contents() { return shared_contents_; }

  void stop_propagation() { stop_propagation_ = true; }
  bool propagation_stopped() const { return stop_propagation_; }

  void ShowDummyView();

 private:
  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DummyContentsData(content::WebContents* dummy_contents,
                    content::WebContents* shared_contents);

  raw_ptr<content::WebContents> dummy_contents_ = nullptr;
  raw_ptr<content::WebContents> shared_contents_ = nullptr;

  bool stop_propagation_ = false;

  std::unique_ptr<SharedPinnedTabDummyView> dummy_view_;
};

////////////////////////////////////////////////////////////////////////////////
// SharedContentsData is a WebContentsUserData attached to pinned tab's web
// contents that could be movable between multiple windows.
//
class SharedContentsData
    : public content::WebContentsUserData<SharedContentsData>,
      public content::WebContentsObserver {
 public:
  SharedContentsData(const SharedContentsData&) = delete;
  SharedContentsData& operator=(const SharedContentsData&) = delete;
  ~SharedContentsData() override {
    while (!dummy_contentses_.empty()) {
      auto* dummy_contents = *dummy_contentses_.begin();
      dummy_contentses_.erase(dummy_contents);
      DummyContentsData::RemoveFromWebContents(dummy_contents);
    }
  }

  static void RemoveFromWebContents(content::WebContents* contents) {
    DCHECK(contents);
    if (FromWebContents(contents)) {
      contents->SetUserData(UserDataKey(), {});
    }
  }

  void AddDummyContents(content::WebContents* contents) {
    dummy_contentses_.insert(contents);
  }

  void RemoveDummyContents(content::WebContents* contents) {
    dummy_contentses_.erase(contents);
  }

 private:
  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  explicit SharedContentsData(content::WebContents* contents)
      : WebContentsUserData(*contents) {
    Observe(contents);
  }

  // In order to detect in-document navigation, which causes url to be changed,
  // use DidFinishNavigation instead of PrimaryPageChanged.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    if (!navigation_handle->IsInMainFrame() ||
        !navigation_handle->HasCommitted()) {
      return;
    }

    for (auto* dummy_contents : dummy_contentses_) {
      auto* dummy_contents_data =
          DummyContentsData::FromWebContents(dummy_contents);
      DCHECK(dummy_contents_data);

      dummy_contents_data->SynchronizeURL();

      Browser* browser = chrome::FindBrowserWithTab(dummy_contents);
      DCHECK(browser);

      // Passing nullptr so that omnibox resets the URL based on the active web
      // contents.
      browser->window()->UpdateToolbar(nullptr);
    }
  }

  base::RepeatingCallback<void(content::WebContents*)> on_primary_page_changed_;

  base::flat_set<content::WebContents*> dummy_contentses_;
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(SharedContentsData);

DummyContentsData::~DummyContentsData() {
  // |shared_contents_data| could be null when the tab is unpinned.
  if (auto* shared_contents_data =
          SharedContentsData::FromWebContents(shared_contents_)) {
    shared_contents_data->RemoveDummyContents(dummy_contents_);
  }
}

// static
void DummyContentsData::RemoveFromWebContents(content::WebContents* contents) {
  DCHECK(contents);
  if (FromWebContents(contents)) {
    contents->SetUserData(UserDataKey(), {});
  }
}

DummyContentsData::DummyContentsData(content::WebContents* dummy_contents,
                                     content::WebContents* shared_contents)
    : WebContentsUserData(*dummy_contents),
      dummy_contents_(dummy_contents),
      shared_contents_(shared_contents) {
  auto* shared_contents_data =
      SharedContentsData::FromWebContents(shared_contents_);
  DCHECK(shared_contents_data);
  shared_contents_data->AddDummyContents(dummy_contents);

  SynchronizeURL();
}

void DummyContentsData::SynchronizeURL() {
  const auto visible_url = shared_contents_->GetVisibleURL();
  dummy_contents_->GetController().GetVisibleEntry()->SetVirtualURL(
      visible_url);
}

void DummyContentsData::ShowDummyView() {
  if (!dummy_view_) {
    dummy_view_ =
        SharedPinnedTabDummyView::Create(shared_contents_, dummy_contents_);
  }

  dummy_view_->Install();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(DummyContentsData);

}  // namespace

SharedPinnedTabService::SharedPinnedTabService(Profile* profile)
    : profile_(profile) {
  DCHECK(base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs));
  profile_observation_.Observe(profile_);

  shared_pinned_tab_enabled_.Init(
      brave_tabs::kSharedPinnedTab, profile_->GetPrefs(),
      base::BindRepeating(&SharedPinnedTabService::OnSharedPinnedTabPrefChanged,
                          weak_ptr_factory_.GetWeakPtr()));

  if (*shared_pinned_tab_enabled_) {
    browser_list_observation_.Observe(BrowserList::GetInstance());
  }
}

SharedPinnedTabService::~SharedPinnedTabService() = default;

bool SharedPinnedTabService::IsSharedContents(
    content::WebContents* contents) const {
  return !!SharedContentsData::FromWebContents(contents);
}

bool SharedPinnedTabService::IsDummyContents(
    content::WebContents* contents) const {
  return !!DummyContentsData::FromWebContents(contents);
}

const TabRendererData*
SharedPinnedTabService::GetTabRendererDataForDummyContents(
    int index,
    content::WebContents* maybe_dummy_contents) {
  auto* dummy_contents_data =
      DummyContentsData::FromWebContents(maybe_dummy_contents);
  DCHECK(dummy_contents_data);

  for (auto& pinned_tab_data : pinned_tab_data_) {
    if (pinned_tab_data.shared_contents ==
        dummy_contents_data->shared_contents()) {
      return &pinned_tab_data.renderer_data;
    }
  }

  NOTREACHED_NORETURN();
}

void SharedPinnedTabService::Shutdown() {
  DCHECK(cached_shared_contentses_from_closing_browser_.empty())
      << " There're dangled web contentses";

  profile_ = nullptr;
  browsers_.clear();
  last_active_browser_ = nullptr;
  closing_browsers_.clear();
  pinned_tab_data_.clear();
  change_source_model_ = nullptr;
  profile_observation_.Reset();
  browser_list_observation_.Reset();
}

void SharedPinnedTabService::OnBrowserAdded(Browser* browser) {
  DVLOG(1) << "Browser added: " << browser->type();
  if (profile_ != browser->profile()) {
    return;
  }

  if (!browser->is_type_normal()) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << " " << browser->tab_strip_model()->count();

  browser->tab_strip_model()->AddObserver(this);
  browsers_.insert(browser);
}

void SharedPinnedTabService::OnBrowserSetLastActive(Browser* browser) {
  if (change_source_model_) {
    // This could happen when a shared web contents is attached to the closing
    // browser. We attach the shared web contents to the closing browser so that
    // it can be restore on the next startup.
    return;
  }

  if (!base::Contains(browsers_, browser)) {
    // Browser could be different profile or not a type we're
    // looking for. Let |OnBrowserAdded| decide which to look for.
    DCHECK(!browser->is_type_normal() || profile_ != browser->profile() ||
           base::Contains(closing_browsers_, browser))
        << "We expect a Browser to be created before set active";
    return;
  }

  auto* model = browser->tab_strip_model();
  DVLOG(2) << __FUNCTION__ << " " << model->count();
  DCHECK_LT(0, model->count()) << "We're assuming that browser has tabs";

  SynchronizeNewBrowser(browser);

  if (last_active_browser_ != browser) {
    last_active_browser_ = browser;
    OnActiveTabChanged(last_active_browser_->tab_strip_model());
  }
}

void SharedPinnedTabService::OnBrowserClosing(Browser* browser) {
  DVLOG(2) << __FUNCTION__;
  if (!base::Contains(browsers_, browser)) {
    // This could be called multiple times for the same |browser|
    return;
  }

  browsers_.erase(browser);
  closing_browsers_.insert(browser);
  if (last_active_browser_ == browser) {
    last_active_browser_ = nullptr;
  }

  if (browsers_.empty()) {
    if (cached_shared_contentses_from_closing_browser_.size()) {
      // This was the last browser and there's a dangling contentses. We should
      // attach them to this |browser| so that they could be cleaned up.
      for (auto i = 0u; i < pinned_tab_data_.size(); i++) {
        if (!pinned_tab_data_.at(i).contents_owner_model ||
            pinned_tab_data_.at(i).contents_owner_model !=
                browser->tab_strip_model()) {
          MoveSharedWebContentsToBrowser(browser, i,
                                         /* is_last_closing_browser */ true);
        }
      }
    }
  } else {
    CHECK(!profile_will_be_destroyed_);

    // Try caching shared contents from the closing browser.
    auto* tab_strip_model = browser->tab_strip_model();
    for (int i = tab_strip_model->IndexOfFirstNonPinnedTab() - 1; i >= 0; --i) {
      auto* web_contents = tab_strip_model->GetWebContentsAt(i);
      if (!web_contents || !SharedContentsData::FromWebContents(web_contents)) {
        continue;
      }

      cached_shared_contentses_from_closing_browser_.insert(
          tab_strip_model->DetachWebContentsAtForInsertion(i));
    }

    for (auto& pinned_tab_data : pinned_tab_data_) {
      if (pinned_tab_data.contents_owner_model == browser->tab_strip_model()) {
        pinned_tab_data.contents_owner_model = nullptr;
      }
    }
  }
}

void SharedPinnedTabService::OnBrowserRemoved(Browser* browser) {
  DVLOG(2) << __FUNCTION__;
  closing_browsers_.erase(browser);

  // On Mac, even after the last browser is closed, the app could be still alive
  // on background. We should clean up the data in this case.
  if (last_active_browser_ == browser) {
    last_active_browser_ = nullptr;
  }

  if (browsers_.empty()) {
    pinned_tab_data_.clear();
  }
}

void SharedPinnedTabService::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (change_source_model_) {
    return;
  }

  if (change.type() == TabStripModelChange::Type::kInserted) {
    OnTabAdded(tab_strip_model, change.GetInsert());
  } else if (change.type() == TabStripModelChange::Type::kRemoved) {
    OnTabRemoved(tab_strip_model, change.GetRemove());
  } else if (change.type() == TabStripModelChange::Type::kMoved) {
    OnTabMoved(tab_strip_model, change.GetMove());
  }
  // TODO(sko) Replace should be handled.

  if (selection.active_tab_changed()) {
    OnActiveTabChanged(tab_strip_model);
  }
}

void SharedPinnedTabService::TabPinnedStateChanged(
    TabStripModel* tab_strip_model,
    content::WebContents* contents,
    int index) {
  if (change_source_model_) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << " index: " << index << " pinned? "
           << tab_strip_model->IsTabPinned(index);
  if (tab_strip_model->IsTabPinned(index)) {
    LOCK_REENTRANCE(tab_strip_model);
    SharedContentsData::CreateForWebContents(contents);
    auto tab_renderer_data =
        TabRendererData::FromTabInModel(tab_strip_model, index);
    DCHECK_LE(index, static_cast<int>(pinned_tab_data_.size()));
    pinned_tab_data_.insert(pinned_tab_data_.begin() + index,
                            {.renderer_data = tab_renderer_data,
                             .shared_contents = contents,
                             .contents_owner_model = tab_strip_model});
    SynchronizeNewPinnedTab(index);
  } else {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&SharedPinnedTabService::OnTabUnpinned,
                       weak_ptr_factory_.GetWeakPtr(), tab_strip_model,
                       contents->GetWeakPtr(), index));
  }
}

void SharedPinnedTabService::TabChangedAt(content::WebContents* contents,
                                          int index,
                                          TabChangeType change_type) {
  if (change_source_model_) {
    return;
  }

  if (DummyContentsData::FromWebContents(contents)) {
    // We don't need to propagate changes from dummy contentses.
    return;
  }

  auto iter = base::ranges::find(pinned_tab_data_, contents,
                                 &PinnedTabData::shared_contents);
  if (iter == pinned_tab_data_.end()) {
    return;
  }

  LOCK_REENTRANCE(iter->contents_owner_model);

  iter->renderer_data =
      TabRendererData::FromTabInModel(iter->contents_owner_model, index);
  for (auto* browser : browsers_) {
    auto* tab_strip_model = browser->tab_strip_model();
    if (tab_strip_model == change_source_model_) {
      continue;
    }

    tab_strip_model->UpdateWebContentsStateAt(index, change_type);
  }
}

void SharedPinnedTabService::OnProfileWillBeDestroyed(Profile* profile) {
  profile_will_be_destroyed_ = true;
}

void SharedPinnedTabService::OnTabAdded(
    TabStripModel* tab_strip_model,
    const TabStripModelChange::Insert* insert) {
  DVLOG(2) << __FUNCTION__;
  LOCK_REENTRANCE(tab_strip_model);
  DCHECK(insert);

  for (const auto& contents_with_index : insert->contents) {
    auto current_index =
        tab_strip_model->GetIndexOfWebContents(contents_with_index.contents);
    if (!tab_strip_model->IsTabPinned(current_index)) {
      continue;
    }

    auto tab_renderer_data =
        TabRendererData::FromTabInModel(tab_strip_model, current_index);
    if (static_cast<int>(pinned_tab_data_.size()) > current_index &&
        pinned_tab_data_.at(current_index).renderer_data.last_committed_url ==
            tab_renderer_data.last_committed_url) {
      DummyContentsData::CreateForWebContents(
          /* dummy_contents= */ contents_with_index.contents,
          /* shared_contents= */ pinned_tab_data_.at(current_index)
              .shared_contents);
      continue;
    }

    SharedContentsData::CreateForWebContents(contents_with_index.contents);
    DCHECK_LE(contents_with_index.index,
              static_cast<int>(pinned_tab_data_.size()));
    pinned_tab_data_.insert(
        pinned_tab_data_.begin() + contents_with_index.index,
        {.renderer_data = tab_renderer_data,
         .shared_contents = contents_with_index.contents.get(),
         .contents_owner_model = tab_strip_model});

    SynchronizeNewPinnedTab(contents_with_index.index);
  }
}

void SharedPinnedTabService::OnTabMoved(TabStripModel* tab_strip_model,
                                        const TabStripModelChange::Move* move) {
  if (!tab_strip_model->IsTabPinned(move->to_index)) {
    return;
  }

  if (static_cast<int>(pinned_tab_data_.size()) <= move->from_index) {
    // This case is where a tab is newly pinned. This case will be dealt in
    // TabPinnedStateChanged().
    return;
  }

  DVLOG(2) << __FUNCTION__;

  LOCK_REENTRANCE(tab_strip_model);

  // Now we're handling a pinned tab's was moved while remaining pinned.
  if (move->from_index < move->to_index) {
    std::rotate(pinned_tab_data_.begin() + move->from_index,
                pinned_tab_data_.begin() + move->from_index + 1,
                pinned_tab_data_.begin() + move->to_index + 1);
  } else {
    std::rotate(pinned_tab_data_.begin() + move->to_index,
                pinned_tab_data_.begin() + move->from_index,
                pinned_tab_data_.begin() + move->from_index + 1);
  }

  SynchronizeMovedPinnedTab(move->from_index, move->to_index);
}

void SharedPinnedTabService::OnTabRemoved(
    TabStripModel* tab_strip_model,
    const TabStripModelChange::Remove* remove) {
  DVLOG(2) << __FUNCTION__;
  DCHECK(remove);

  if (base::Contains(closing_browsers_, tab_strip_model,
                     &Browser::tab_strip_model)) {
    // We don't close pinned tabs if this browser is being closed.
    return;
  }

  LOCK_REENTRANCE(tab_strip_model);

  for (const auto& removed_tab : remove->contents) {
    if (removed_tab.index >= static_cast<int>(pinned_tab_data_.size())) {
      // This was non pinned tab
      continue;
    }

    if (!IsDummyContents(removed_tab.contents) &&
        !IsSharedContents(removed_tab.contents)) {
      // This tab is not under our interest. It could be in the middle of
      // destruction  as a result of synchronization.
      continue;
    }

    if (auto* dummy_contents_data =
            DummyContentsData::FromWebContents(removed_tab.contents);
        dummy_contents_data && dummy_contents_data->propagation_stopped()) {
      // We're restoring from cached contents removes dummy pinned tabs, or
      // closing tabs as a result of synchronization.
      // In this case, we shouldn't touch data.
      continue;
    }

    pinned_tab_data_.erase(pinned_tab_data_.begin() + removed_tab.index);
    SynchronizeDeletedPinnedTab(removed_tab.index);
  }
}

void SharedPinnedTabService::OnActiveTabChanged(
    TabStripModel* tab_strip_model) {
  if (change_source_model_) {
    return;
  }

  DVLOG(2) << __FUNCTION__;

  // Swap the pinned active web contents with the shared web contents if needed
  if (!last_active_browser_ ||
      last_active_browser_->tab_strip_model() != tab_strip_model) {
    return;
  }

  if (base::Contains(closing_browsers_, tab_strip_model,
                     &Browser::tab_strip_model)) {
    return;
  }

  const auto active_index = tab_strip_model->active_index();
  if (active_index == TabStripModel::kNoTab) {
    // The browser could be in the middle of shutdown
    return;
  }

  if (!tab_strip_model->IsTabPinned(active_index)) {
    return;
  }

  if (pinned_tab_data_.at(active_index).contents_owner_model ==
      tab_strip_model) {
    // this pinned tab already has the shared contents.
    DCHECK_EQ(pinned_tab_data_.at(active_index).shared_contents,
              tab_strip_model->GetWebContentsAt(active_index));
    return;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &SharedPinnedTabService::MoveSharedWebContentsToActiveBrowser,
          weak_ptr_factory_.GetWeakPtr(), active_index));
}

void SharedPinnedTabService::OnTabUnpinned(
    TabStripModel* tab_strip_model,
    base::WeakPtr<content::WebContents> contents,
    int index) {
  DCHECK(contents);

  LOCK_REENTRANCE(tab_strip_model);

  auto* shared_contents = contents.get();
  if (auto* dummy_contents_data =
          DummyContentsData::FromWebContents(contents.get())) {
    shared_contents = dummy_contents_data->shared_contents();
  }

  // We shouldn't count on |index| in this case. The previous index could be
  // different from |index|.
  auto iter = base::ranges::find(pinned_tab_data_, shared_contents,
                                 &PinnedTabData::shared_contents);
  DCHECK(iter != pinned_tab_data_.end());
  const auto previous_index = std::distance(pinned_tab_data_.begin(), iter);

  if (auto* dummy_contents_data =
          DummyContentsData::FromWebContents(contents.get())) {
    // We should make this |tab_strip_model| have the shared contents, as
    // other tabs will be deleted soon.
    dummy_contents_data->stop_propagation();

    DCHECK_NE(iter->contents_owner_model, tab_strip_model);

    auto unique_shared_contents =
        iter->contents_owner_model->DiscardWebContentsAt(
            previous_index, CreateDummyWebContents(shared_contents));
    SharedContentsData::RemoveFromWebContents(unique_shared_contents.get());

    tab_strip_model->DiscardWebContentsAt(index,
                                          std::move(unique_shared_contents));
  } else {
    SharedContentsData::RemoveFromWebContents(contents.get());
  }

  pinned_tab_data_.erase(iter);

  SynchronizeDeletedPinnedTab(previous_index);
}

void SharedPinnedTabService::SynchronizeNewPinnedTab(int index) {
  DVLOG(2) << __FUNCTION__;
  DCHECK_LT(index, static_cast<int>(pinned_tab_data_.size()));
  DCHECK(change_source_model_);

  for (auto* browser : browsers_) {
    auto* model = browser->tab_strip_model();
    if (model == change_source_model_) {
      continue;
    }

    model->InsertWebContentsAt(
        index,
        CreateDummyWebContents(pinned_tab_data_.at(index).shared_contents),
        ADD_PINNED | ADD_FORCE_INDEX);
  }
}

void SharedPinnedTabService::SynchronizeDeletedPinnedTab(int index) {
  DVLOG(2) << __FUNCTION__;
  DCHECK(change_source_model_);

  for (auto* browser : browsers_) {
    auto* model = browser->tab_strip_model();
    if (model == change_source_model_) {
      continue;
    }

    // We may not want to keep history for dummy pinned tabs, so pass 0 for
    // |close_type|.
    model->CloseWebContentsAt(index, /* close_type */ 0);
  }
}

void SharedPinnedTabService::SynchronizeMovedPinnedTab(int from, int to) {
  DVLOG(2) << __FUNCTION__;
  DCHECK(change_source_model_);

  for (auto* browser : browsers_) {
    auto* model = browser->tab_strip_model();
    if (model == change_source_model_) {
      continue;
    }

    model->MoveWebContentsAt(from, to, /* select_after_move= */ false);
  }
}

void SharedPinnedTabService::TabDraggingEnded(Browser* browser) {
  if (!in_tab_dragging_browsers_.erase(browser)) {
    return;
  }

  if (!browser->IsBrowserClosing()) {
    SynchronizeNewBrowser(browser);
  }
}

void SharedPinnedTabService::SynchronizeNewBrowser(Browser* browser) {
  if (IsBrowserInTabDragging(browser)) {
    in_tab_dragging_browsers_.insert(browser);
    return;
  }

  auto* model = browser->tab_strip_model();
  std::vector<PinnedTabData> new_pinned_tabs;
  for (auto i = 0; i < model->IndexOfFirstNonPinnedTab(); i++) {
    new_pinned_tabs.push_back(
        {.renderer_data = TabRendererData::FromTabInModel(model, i),
         .shared_contents = model->GetWebContentsAt(i),
         .contents_owner_model = model});
  }

  if (base::ranges::equal(
          new_pinned_tabs, pinned_tab_data_,
          [&](const auto& new_pin_tab, const auto& old_pin_tab) {
            return new_pin_tab.renderer_data.last_committed_url ==
                   old_pin_tab.renderer_data.last_committed_url;
          })) {
    DVLOG(2) << __FUNCTION__ << " Shared pinned tabs in sync";
    // Tabs are already in sync. Check web contents data is attached properly
    // in case they were restored from a session.
    for (auto i = 0u; i < pinned_tab_data_.size(); i++) {
      content::WebContents* contents = new_pinned_tabs.at(i).shared_contents;
      if (IsSharedContents(contents)) {
        continue;
      }

      if (!IsDummyContents(contents)) {
        DummyContentsData::CreateForWebContents(
            /* dummy_contents= */ new_pinned_tabs.at(i).shared_contents,
            /* shared_contents= */ pinned_tab_data_.at(i).shared_contents);
      }
    }
    return;
  }

  // Add shared pinned tabs to |browser| first.
  LOCK_REENTRANCE(model);

  for (auto i = 0u; i < pinned_tab_data_.size(); i++) {
    model->InsertWebContentsAt(
        i, CreateDummyWebContents(pinned_tab_data_[i].shared_contents),
        ADD_PINNED | ADD_FORCE_INDEX);
  }

  if (new_pinned_tabs.empty()) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << " Append new shared pinned tabs";
  // If the |browser| has pinned tabs out of sync, append them to other browsers
  const auto offset = pinned_tab_data_.size();
  for (auto i = 0u; i < new_pinned_tabs.size(); i++) {
    SharedContentsData::CreateForWebContents(
        new_pinned_tabs.at(i).shared_contents);
    pinned_tab_data_.push_back(std::move(new_pinned_tabs.at(i)));
    SynchronizeNewPinnedTab(offset + i);
  }
}

void SharedPinnedTabService::MoveSharedWebContentsToActiveBrowser(int index) {
  if (!last_active_browser_) {
    DVLOG(2)
        << "Failed to attach contents to active browser: No active browser";
    return;
  }

  MoveSharedWebContentsToBrowser(last_active_browser_, index);
}

void SharedPinnedTabService::MoveSharedWebContentsToBrowser(
    Browser* browser,
    int index,
    bool is_last_closing_browser) {
  if (IsBrowserInTabDragging(browser)) {
    return;
  }

  auto* tab_strip_model = browser->tab_strip_model();
  DCHECK_LT(index, tab_strip_model->count());

  LOCK_REENTRANCE(tab_strip_model);

  auto& pinned_tab_data = pinned_tab_data_.at(index);
  if (pinned_tab_data.contents_owner_model) {
    // Detach shared pinned tab from the current owner model.
    std::unique_ptr<content::WebContents> unique_shared_contents =
        pinned_tab_data.contents_owner_model->DiscardWebContentsAt(
            index, CreateDummyWebContents(pinned_tab_data.shared_contents));
    DCHECK_EQ(pinned_tab_data.shared_contents, unique_shared_contents.get());

    // Install DummyView to the dummy contents.
    if (pinned_tab_data.contents_owner_model->active_index() == index) {
      DummyContentsData::FromWebContents(
          pinned_tab_data.contents_owner_model->GetWebContentsAt(index))
          ->ShowDummyView();
    }

    // Replace owner model
    pinned_tab_data.contents_owner_model = tab_strip_model;

    // Suppress events from dummy contents that is about to be replaced with
    // the shared pinned tab.
    auto* dummy_contents =
        pinned_tab_data.contents_owner_model->GetWebContentsAt(index);
    auto* dummy_contents_data =
        DummyContentsData::FromWebContents(dummy_contents);
    DCHECK(dummy_contents_data);
    dummy_contents_data->stop_propagation();

    auto discarded_content =
        pinned_tab_data.contents_owner_model->DiscardWebContentsAt(
            index, std::move(unique_shared_contents));
    // Need to clear tab interface before it's gone. Otherwise,
    // EmbeddingTabTracker will have dangling pointer to TabInterface.
    webui::SetTabInterface(discarded_content.get(), nullptr);
  } else {
    // Restore a shared pinned tab from a closed browser.
    auto iter =
        base::ranges::find(cached_shared_contentses_from_closing_browser_,
                           pinned_tab_data.shared_contents,
                           &std::unique_ptr<content::WebContents>::get);
    DCHECK(iter != cached_shared_contentses_from_closing_browser_.end());
    auto unique_shared_contents = std::move(*iter);
    DCHECK(unique_shared_contents);

    cached_shared_contentses_from_closing_browser_.erase(iter);

    pinned_tab_data.contents_owner_model = tab_strip_model;

    auto* dummy_contents_data = DummyContentsData::FromWebContents(
        tab_strip_model->GetWebContentsAt(index));
    DCHECK(dummy_contents_data);
    dummy_contents_data->stop_propagation();

    const int add_type =
        ADD_PINNED | (is_last_closing_browser ? ADD_ACTIVE : 0);
    tab_strip_model->InsertWebContentsAt(
        index, std::move(unique_shared_contents), add_type);

    // In order to prevent browser from being closed, we should close the dummy
    // contents after we restore the tab.
    tab_strip_model->CloseWebContentsAt(index + 1, /* close_type */ 0);
  }
}

void SharedPinnedTabService::OnSharedPinnedTabPrefChanged() {
  if (*shared_pinned_tab_enabled_) {
    OnSharedPinnedTabEnabled();
  } else {
    OnSharedPinnedTabDisabled();
  }
}

void SharedPinnedTabService::OnSharedPinnedTabEnabled() {
  // Init observers
  browser_list_observation_.Observe(BrowserList::GetInstance());
  auto browsers = chrome::FindAllTabbedBrowsersWithProfile(profile_);
  for (auto* browser : browsers) {
    OnBrowserAdded(browser);
  }

  // Synchronize all pre-existing pinned tabs.
  for (auto* browser : browsers) {
    auto* tab_strip_model = browser->tab_strip_model();
    for (int i = 0; i < tab_strip_model->IndexOfFirstNonPinnedTab(); ++i) {
      auto* contents = tab_strip_model->GetWebContentsAt(i);
      if (IsDummyContents(contents)) {
        // This tab is dummy tab created inside this loop from another
        // browser.
        continue;
      }
      TabPinnedStateChanged(tab_strip_model, contents, i);
    }
  }
}

void SharedPinnedTabService::OnSharedPinnedTabDisabled() {
  // Reset observers. Note that we should remove observers first so that
  // closing dummy contents won't close shared contents too.
  for (auto* browser : browsers_) {
    browser->tab_strip_model()->RemoveObserver(this);
  }
  browser_list_observation_.Reset();

  // Remove all dummy contents
  for (auto* browser : browsers_) {
    auto* tab_strip_model = browser->tab_strip_model();
    for (auto i = tab_strip_model->IndexOfFirstNonPinnedTab() - 1; i >= 0;
         --i) {
      if (IsDummyContents(tab_strip_model->GetWebContentsAt(i))) {
        tab_strip_model->CloseWebContentsAt(i, /* close_type */ 0);
      }
    }
  }

  // Reset data
  browsers_.clear();
  last_active_browser_ = nullptr;
  closing_browsers_.clear();
  pinned_tab_data_.clear();
}

std::unique_ptr<content::WebContents>
SharedPinnedTabService::CreateDummyWebContents(
    content::WebContents* shared_contents) {
  content::WebContents::CreateParams create_params(profile_);
  create_params.initially_hidden = true;
  create_params.desired_renderer_state =
      content::WebContents::CreateParams::kNoRendererProcess;
  auto dummy_contents = content::WebContents::Create(create_params);
  DummyContentsData::CreateForWebContents(dummy_contents.get(),
                                          shared_contents);
  return dummy_contents;
}

bool SharedPinnedTabService::IsBrowserInTabDragging(Browser* browser) const {
  CHECK(browser);
  return static_cast<BraveBrowserWindow*>(browser->window())->IsInTabDragging();
}
