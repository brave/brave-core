/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_H_

#include <memory>
#include <optional>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_service_observer.h"

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

// Manage sidebar's runtime state for active index and icons.
// Each browser window has different runtime state.
// Observe SidebarService to get item add/deletion notification.
class SidebarModel : public SidebarService::Observer,
                     public history::HistoryServiceObserver {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnItemAdded(const SidebarItem& item,
                             size_t index,
                             bool user_gesture) {}
    virtual void OnItemMoved(const SidebarItem& item, size_t from, size_t to) {}
    virtual void OnWillRemoveItem(const SidebarItem& item) {}
    virtual void OnItemRemoved(size_t index) {}
    virtual void OnActiveIndexChanged(std::optional<size_t> old_index,
                                      std::optional<size_t> new_index) {}
    virtual void OnItemUpdated(const SidebarItem& item,
                               const SidebarItemUpdate& update) {}
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

  void SetActiveIndex(std::optional<size_t> index);
  // Returns true if webcontents of item at |index| already loaded url.
  bool IsSidebarHasAllBuiltInItems() const;
  std::optional<size_t> GetIndexOf(const SidebarItem& item) const;
  std::optional<size_t> GetIndexOf(SidebarItem::BuiltInItemType type) const;

  void FetchFavicon(const sidebar::SidebarItem& item);

  // Don't cache item list. list can be changed during the runtime.
  const std::vector<SidebarItem>& GetAllSidebarItems() const;

  // Return std::nullopt if sidebar panel is not opened.
  std::optional<size_t> active_index() const { return active_index_; }

  // SidebarService::Observer overrides:
  void OnItemAdded(const SidebarItem& item, size_t index) override;
  void OnItemMoved(const SidebarItem& item, size_t from, size_t to) override;
  void OnItemUpdated(const SidebarItem& item,
                     const SidebarItemUpdate& update) override;
  void OnWillRemoveItem(const SidebarItem& item, size_t index) override;
  void OnItemRemoved(const SidebarItem& item, size_t index) override;

  // history::HistoryServiceObserver overrides:
  void OnURLVisited(history::HistoryService* history_service,
                    const history::URLRow& url_row,
                    const history::VisitRow& new_visit) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SidebarModelTest, ItemsChangedTest);
  FRIEND_TEST_ALL_PREFIXES(SidebarModelTest, ActiveIndexChangedAfterItemAdded);

  // Add item at last.
  void AddItem(const SidebarItem& item, size_t index, bool user_gesture);
  void RemoveItemAt(size_t index);
  void UpdateActiveIndexAndNotify(std::optional<size_t> new_active_index);

  // TODO(simonhong): Use separated class for fetching favicon from this model
  // class.
  void OnGetLocalFaviconImage(
      const sidebar::SidebarItem& item,
      const favicon_base::FaviconRawBitmapResult& bitmapt_result);

  void FetchFaviconFromNetwork(const sidebar::SidebarItem& item);
  void OnGetFaviconImageFromNetwork(
      const sidebar::SidebarItem& item,
      const gfx::Image& image,
      const image_fetcher::RequestMetadata& request_metadata);

  // Optional engaged if sidebar panel is opened.
  std::optional<size_t> active_index_ = std::nullopt;
  const raw_ptr<Profile> profile_ = nullptr;
  std::unique_ptr<base::CancelableTaskTracker> task_tracker_;
  base::ObserverList<Observer> observers_;
  base::ScopedObservation<SidebarService, SidebarService::Observer>
      sidebar_observed_{this};
  base::ScopedObservation<history::HistoryService, HistoryServiceObserver>
      history_observed_{this};
  base::WeakPtrFactory<SidebarModel> weak_ptr_factory_{this};
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_H_
