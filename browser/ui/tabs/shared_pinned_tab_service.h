/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_SERVICE_H_
#define BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_SERVICE_H_

#include <memory>
#include <vector>

#include "base/scoped_observation.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_renderer_data.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;
class BrowserList;

// SharedPinnedTabService observes pinned tabs and synchronizes it to all
// windows with the same profile. When a pinned tab in a window is activated,
// the pinned tab's web contents could be replaced with a web contents that
// user was interacting with in another window. i.e. This service will make
// pinned tabs to share a single web contents instance.
// This service will be created when profile is created, so we don't have to
// create this explicitly.
class SharedPinnedTabService : public KeyedService,
                               public BrowserListObserver,
                               public TabStripModelObserver,
                               public ProfileObserver {
 public:
  struct PinnedTabData {
    TabRendererData renderer_data;
    raw_ptr<content::WebContents> shared_contents = nullptr;
    raw_ptr<TabStripModel> contents_owner_model = nullptr;
  };

  explicit SharedPinnedTabService(Profile* profile);
  SharedPinnedTabService(const SharedPinnedTabService&) = delete;
  SharedPinnedTabService& operator=(const SharedPinnedTabService&) = delete;
  ~SharedPinnedTabService() override;

  // There are two types of contents for pinned tabs. "Shared contents" is a
  // single instance contents for a pinned tab shared across multiple windows.
  // "Dummy contents" is a empty contents used as a placeholder for inactive
  // windows.
  bool IsSharedContents(content::WebContents* contents) const;
  bool IsDummyContents(content::WebContents* contents) const;

  // Returns nullptr if it's not dummy contents or the data isn't ready yet.
  const TabRendererData* GetTabRendererDataForDummyContents(
      int index,
      content::WebContents* maybe_dummy_contents);

  void CacheWebContentsIfNeeded(
      Browser* browser,
      const std::vector<std::unique_ptr<TabStripModel::DetachedWebContents>>&
          web_contents);

  // KeyedService:
  void Shutdown() override;

  // BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserClosing(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index) override;
  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change_type) override;

  // ProfileWillBeDestroyed;
  void OnProfileWillBeDestroyed(Profile* profile) override;

 private:
  void OnTabAdded(TabStripModel* tab_strip_model,
                  const TabStripModelChange::Insert* insert);
  void OnTabMoved(TabStripModel* tab_strip_model,
                  const TabStripModelChange::Move* move);
  void OnTabRemoved(TabStripModel* tab_strip_model,
                    const TabStripModelChange::Remove* remove);
  void OnActiveTabChanged(TabStripModel* tab_strip_model);

  void OnTabUnpinned(TabStripModel* tab_strip_model,
                     base::WeakPtr<content::WebContents> contents,
                     int index);

  void SynchronizeNewPinnedTab(int index);
  void SynchronizeDeletedPinnedTab(int index);
  void SynchronizeMovedPinnedTab(int from, int to);
  void SynchronizeNewBrowser(Browser* browser);

  void MoveSharedWebContentsToActiveBrowser(int index);
  void MoveSharedWebContentsToBrowser(Browser* browser, int index);

  std::unique_ptr<content::WebContents> CreateDummyWebContents(
      content::WebContents* shared_contents);

  raw_ptr<Profile> profile_;

  base::flat_set<Browser*> browsers_;
  raw_ptr<Browser> last_active_browser_ = nullptr;

  base::flat_set<Browser*> closing_browsers_;
  base::flat_set<std::unique_ptr<content::WebContents>>
      cached_shared_contentses_from_closing_browser_;

  // This data is ordered in the actual pinned tab order.
  std::vector<PinnedTabData> pinned_tab_data_;

  raw_ptr<TabStripModel> change_source_model_ = nullptr;

  bool profile_will_be_destroyed_ = false;

  base::ScopedObservation<Profile, ProfileObserver> profile_observation_{this};

  base::ScopedObservation<BrowserList, BrowserListObserver>
      browser_list_observation_{this};

  base::WeakPtrFactory<SharedPinnedTabService> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_SERVICE_H_
