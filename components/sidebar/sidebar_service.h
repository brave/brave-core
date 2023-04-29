/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_SERVICE_H_
#define BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/version_info/channel.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefRegistrySimple;
class PrefService;

namespace sidebar {

class SidebarServiceDelegate;

struct SidebarItemUpdate {
  size_t index = 0;
  bool title_updated = false;
  bool url_updated = false;

  bool operator==(const SidebarItemUpdate& update) const;
};

// This manages per-context persisted sidebar items list.
class SidebarService : public KeyedService {
 public:
  enum class ShowSidebarOption {
    kShowAlways = 0,
    kShowOnMouseOver,
    kShowOnClick,  // Don't use. Deprecated.
    kShowNever,
  };

  // This is the default display order
  static constexpr SidebarItem::BuiltInItemType kDefaultBuiltInItemTypes[] = {
      SidebarItem::BuiltInItemType::kBraveTalk,
      SidebarItem::BuiltInItemType::kWallet,
      SidebarItem::BuiltInItemType::kBookmarks,
      SidebarItem::BuiltInItemType::kReadingList,
      SidebarItem::BuiltInItemType::kHistory,
      SidebarItem::BuiltInItemType::kPlaylist,
      SidebarItem::BuiltInItemType::kChatUI};
  static_assert(
      std::size(kDefaultBuiltInItemTypes) ==
          static_cast<size_t>(SidebarItem::BuiltInItemType::kBuiltInItemLast),
      "A built-in item in this visual order is missing or you might forget to "
      "update kBuiltInItemItemLast value. If you want to add a "
      "new item while keeping that hidden, please visit "
      "GetBuiltInItemForType() in sidebar_service.cc");

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnItemAdded(const SidebarItem& item, size_t index) {}
    virtual void OnItemMoved(const SidebarItem& item, size_t from, size_t to) {}
    virtual void OnWillRemoveItem(const SidebarItem& item, size_t index) {}
    virtual void OnItemRemoved(const SidebarItem& item, size_t index) {}
    virtual void OnItemUpdated(const SidebarItem& item,
                               const SidebarItemUpdate& update) {}
    virtual void OnShowSidebarOptionChanged(ShowSidebarOption option) {}

   protected:
    ~Observer() override = default;
  };

  static void RegisterProfilePrefs(PrefRegistrySimple* registry,
                                   version_info::Channel channel);

  SidebarService(PrefService* prefs,
                 std::unique_ptr<SidebarServiceDelegate> delegate);
  ~SidebarService() override;

  const std::vector<SidebarItem>& items() const { return items_; }

  void AddItem(const SidebarItem& item);
  void RemoveItemAt(int index);
  void MoveItem(size_t from, size_t to);

  // Only non-builtin type is editable.
  // URL acts like an id for each item.
  void UpdateItem(const GURL& old_url,
                  const GURL& new_url,
                  const std::u16string& old_title,
                  const std::u16string& new_title);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  std::vector<SidebarItem> GetHiddenDefaultSidebarItems() const;
  ShowSidebarOption GetSidebarShowOption() const;
  void SetSidebarShowOption(ShowSidebarOption show_options);

  void MoveSidebarToRightTemporarily();
  void RestoreSidebarAlignmentIfNeeded();

  absl::optional<SidebarItem> GetDefaultPanelItem() const;
  bool IsEditableItemAt(size_t index) const;

  SidebarService(const SidebarService&) = delete;
  SidebarService& operator=(const SidebarService&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(SidebarServiceTest, AddRemoveItems);

  void LoadSidebarItems();
  void UpdateSidebarItemsToPrefStore();
  std::vector<SidebarItem> GetDefaultSidebarItems() const;
  SidebarItem GetBuiltInItemForType(SidebarItem::BuiltInItemType type) const;
  std::vector<SidebarItem::BuiltInItemType> GetCurrentlyPresentBuiltInTypes()
      const;
  void OnPreferenceChanged(const std::string& pref_name);
  void MigrateSidebarShowOptions();
  void MigratePrefSidebarBuiltInItemsToHidden();

  raw_ptr<PrefService> prefs_ = nullptr;

  std::unique_ptr<SidebarServiceDelegate> delegate_;

  std::vector<SidebarItem> items_;

  base::ObserverList<Observer> observers_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_SERVICE_H_
