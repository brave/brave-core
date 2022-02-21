/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_H_

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/sidebar/sidebar_model_data.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_service_observer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class CancelableTaskTracker;
}  // namespace base

namespace favicon_base {
struct FaviconRawBitmapResult;
}  // namespace favicon_base

namespace gfx {
class Image;
class ImageSkia;
}  // namespace gfx

namespace image_fetcher {
struct RequestMetadata;
}  // namespace image_fetcher

namespace history {
class HistoryService;
}  // namespace history

class Profile;

namespace sidebar {

class SidebarModelData;

// Manage sidebar's runtime state.
// Each browser window has different runtime state.
// Observe SidebarService to get item add/deletion notification.
class SidebarModel : public SidebarService::Observer,
                     public history::HistoryServiceObserver {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnItemAdded(const SidebarItem& item,
                             int index,
                             bool user_gesture) {}
    virtual void OnItemMoved(const SidebarItem& item, int from, int to) {}
    virtual void OnItemRemoved(int index) {}
    virtual void OnActiveIndexChanged(int old_index, int new_index) {}
    virtual void OnFaviconUpdatedForItem(const SidebarItem& item,
                                         const gfx::ImageSkia& image) {}

   protected:
    ~Observer() override = default;
  };

  explicit SidebarModel(Profile* profile);
  ~SidebarModel() override;

  SidebarModel(const SidebarModel&) = delete;
  SidebarModel& operator=(const SidebarModel&) = delete;

  void Init(history::HistoryService* history_service);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // |false| is used in unit test.
  void SetActiveIndex(int index, bool load = true);
  // Returns true if webcontents of item at |index| already loaded url.
  bool IsLoadedAt(int index) const;
  bool IsSidebarHasAllBuiltiInItems() const;
  int GetIndexOf(const SidebarItem& item) const;

  // Don't cache web_contents. It can be deleted during the runtime.
  content::WebContents* GetWebContentsAt(int index);
  // Returns true when |web_contents| is used by sidebar panel.
  bool IsSidebarWebContents(const content::WebContents* web_contents) const;

  // Don't cache item list. list can be changed during the runtime.
  const std::vector<SidebarItem> GetAllSidebarItems() const;

  // Return -1 if sidebar panel is not opened.
  int active_index() const { return active_index_; }

  // SidebarService::Observer overrides:
  void OnItemAdded(const SidebarItem& item, int index) override;
  void OnItemMoved(const SidebarItem& item, int from, int to) override;
  void OnWillRemoveItem(const SidebarItem& item, int index) override;
  void OnItemRemoved(const SidebarItem& item, int index) override;

  // history::HistoryServiceObserver overrides:
  void OnURLVisited(history::HistoryService* history_service,
                    ui::PageTransition transition,
                    const history::URLRow& row,
                    const history::RedirectList& redirects,
                    base::Time visit_time) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SidebarModelTest, ItemsChangedTest);

  // Add item at last.
  void AddItem(const SidebarItem& item, int index, bool user_gesture);
  void RemoveItemAt(int index);
  void UpdateActiveIndexAndNotify(int new_active_index);
  void LoadURLAt(const GURL& url, int index);

  // TODO(simonhong): Use separated class for fetching favicon from this model
  // class.
  void FetchFavicon(const sidebar::SidebarItem& item);
  void OnGetLocalFaviconImage(
      const sidebar::SidebarItem& item,
      const favicon_base::FaviconRawBitmapResult& bitmapt_result);

  void FetchFaviconFromNetwork(const sidebar::SidebarItem& item);
  void OnGetFaviconImageFromNetwork(
      const sidebar::SidebarItem& item,
      const gfx::Image& image,
      const image_fetcher::RequestMetadata& request_metadata);

  // Non-negative if sidebar panel is opened.
  int active_index_ = -1;
  Profile* profile_ = nullptr;
  std::unique_ptr<base::CancelableTaskTracker> task_tracker_;
  base::ObserverList<Observer> observers_;
  std::vector<std::unique_ptr<SidebarModelData>> data_;
  base::ScopedObservation<SidebarService, SidebarService::Observer>
      sidebar_observed_{this};
  base::ScopedObservation<history::HistoryService, HistoryServiceObserver>
      history_observed_{this};
  base::WeakPtrFactory<SidebarModel> weak_ptr_factory_{this};
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_H_
