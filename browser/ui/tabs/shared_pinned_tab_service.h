/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_SERVICE_H_
#define BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_SERVICE_H_

#include <memory>
#include <optional>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "chrome/browser/ui/browser_window/public/browser_collection_observer.h"
#include "chrome/browser/ui/tabs/tab_data.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_member.h"

class Browser;
class Profile;
class BrowserWindowInterface;
class BrowserCollection;

// SharedPinnedTabService observes pinned tabs and synchronizes it to all
// windows with the same profile. When a pinned tab in a window is activated,
// the pinned tab's web contents could be replaced with a web contents that
// user was interacting with in another window. i.e. This service will make
// pinned tabs to share a single web contents instance.
// This service will be created when profile is created, so we don't have to
// create this explicitly.
//
// Indices come in two flavors, named accordingly throughout this class:
// "raw" indices are positions within one window's TabStripModel; "canonical"
// indices are positions within |pinned_tab_data_|. The two only differ when
// that window has an excluded pinned tab (see FindExcludedPinnedIndex());
// ToCanonicalIndex()/ToRawIndex() convert between them.
class SharedPinnedTabService : public KeyedService,
                               public BrowserCollectionObserver,
                               public TabStripModelObserver,
                               public ProfileObserver {
 public:
  struct PinnedTabData {
    tabs::TabData renderer_data;
    raw_ptr<content::WebContents, DanglingUntriaged> shared_contents = nullptr;
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
  const tabs::TabData* GetTabDataForDummyContents(
      int raw_index,
      content::WebContents* maybe_dummy_contents);

  void TabDraggingEnded(BrowserWindowInterface* browser);

  // Returns true if the browser will be closed by this method. If true,
  // Browser::OnWindowClosing() should not proceed, as TabStrip::Empty() will
  // be invoked again.
  [[nodiscard]] bool BrowserClosing(TabStripModel* tab_strip_model);

  // KeyedService:
  void Shutdown() override;

  // BrowserCollectionObserver:
  void OnBrowserCreated(BrowserWindowInterface* browser) override;
  void OnBrowserActivated(BrowserWindowInterface* browser) override;
  void OnBrowserClosed(BrowserWindowInterface* browser) override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  void OnTabPinnedStateChanged(tabs::TabInterface* tab, int raw_index) override;
  void OnTabChangedAt(tabs::TabInterface* tab,
                      TabChangeType change_type) override;
  void OnTabBlockedStateChanged(tabs::TabInterface* tab) override;
  void OnTabStripModelDestroyed(TabStripModel* tab_strip_model) override;

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
                     int raw_index);

  void SynchronizeNewPinnedTab(int canonical_index);
  void SynchronizeDeletedPinnedTab(int canonical_index);
  void SynchronizeMovedPinnedTab(int canonical_from, int canonical_to);
  void SynchronizeNewBrowser(BrowserWindowInterface* browser);

  // Retries SynchronizeNewBrowser() for a browser whose synchronization was
  // deferred behind a pending unpin.
  void SynchronizeDeferredBrowser(
      base::WeakPtr<BrowserWindowInterface> browser);

  void MoveSharedWebContentsToActiveBrowser(
      base::WeakPtr<content::WebContents> contents);
  void MoveSharedWebContentsToBrowser(BrowserWindowInterface* browser,
                                      int canonical_index,
                                      bool is_last_closing_browser = false);

  void OnSharedPinnedTabPrefChanged();
  void OnSharedPinnedTabEnabled();
  void OnSharedPinnedTabDisabled();

  std::unique_ptr<content::WebContents> CreateDummyWebContents(
      content::WebContents* shared_contents);

  bool IsBrowserInTabDragging(BrowserWindowInterface* browser) const;

  // Whether |contents| is |shared_contents| itself or a dummy for it.
  bool IsTrackedContentsFor(content::WebContents* contents,
                            content::WebContents* shared_contents) const;

  // A window may contain one pinned tab that must never be tracked in
  // |pinned_tab_data_| (see SharedPinnedTabExclusionMarker), which offsets
  // every raw pinned-tab index in that window relative to |pinned_tab_data_|.
  // These three helpers translate between the two index spaces.

  // Raw pinned-tab index of |model|'s excluded tab, or std::nullopt if it has
  // none. Assume at most one excluded pinned tab in the tab strip model.
  std::optional<int> FindExcludedPinnedIndex(TabStripModel* model) const;

  // Raw pinned-tab index in |model| (must not be the excluded tab's own
  // index) -> the corresponding index into |pinned_tab_data_|.
  int ToCanonicalIndex(TabStripModel* model, int raw_index) const;

  // Index into |pinned_tab_data_| -> the raw pinned-tab index to use in
  // |model|, accounting for |model|'s own excluded tab (if any).
  int ToRawIndex(TabStripModel* model, int canonical_index) const;

  raw_ptr<Profile> profile_;

  base::flat_set<raw_ptr<BrowserWindowInterface>> browsers_;
  raw_ptr<BrowserWindowInterface> last_active_browser_ = nullptr;

  base::flat_set<raw_ptr<BrowserWindowInterface>> closing_browsers_;
  base::flat_set<std::unique_ptr<content::WebContents>>
      cached_shared_contentses_from_closing_browser_;
  base::flat_set<raw_ptr<BrowserWindowInterface>> in_tab_dragging_browsers_;

  // This data is ordered in the actual pinned tab order.
  std::vector<PinnedTabData> pinned_tab_data_;

  raw_ptr<TabStripModel> change_source_model_ = nullptr;

  // Tab strips with an unpin whose OnTabUnpinned() is posted but hasn't run
  // yet. Until it runs, such a tab strip has fewer pinned tabs than
  // |pinned_tab_data_|, which must not be mistaken for an out-of-sync browser.
  // There is one entry per pending unpin, so a model may appear more than once.
  std::vector<raw_ptr<TabStripModel>> pending_unpin_models_;

  bool profile_will_be_destroyed_ = false;

  base::ScopedObservation<Profile, ProfileObserver> profile_observation_{this};

  base::ScopedObservation<BrowserCollection, BrowserCollectionObserver>
      browser_collection_observation_{this};

  BooleanPrefMember shared_pinned_tab_enabled_;

  base::WeakPtrFactory<SharedPinnedTabService> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_SERVICE_H_
