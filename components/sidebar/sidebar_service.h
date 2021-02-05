/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_SERVICE_H_
#define BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_SERVICE_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefRegistrySimple;
class PrefService;

namespace sidebar {

// This manages per-context persisted sidebar items list.
class SidebarService : public KeyedService {
 public:
  enum ShowSidebarOption {
    kShowAlways = 0,
    kShowOnMouseOver,
    kShowOnClick,
    kShowNever,
  };

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnItemAdded(const SidebarItem& item, int index) {}
    virtual void OnWillRemoveItem(const SidebarItem& item, int index) {}
    virtual void OnItemRemoved(const SidebarItem& item, int index) {}
    virtual void OnShowSidebarOptionChanged(int option) {}

   protected:
    ~Observer() override = default;
  };

  static void RegisterPrefs(PrefRegistrySimple* registry);

  explicit SidebarService(PrefService* prefs);
  ~SidebarService() override;

  const std::vector<SidebarItem> items() const { return items_; }

  void AddItem(const SidebarItem& item);
  void RemoveItemAt(int index);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  std::vector<SidebarItem> GetNotAddedDefaultSidebarItems() const;
  int GetSidebarShowOption() const;
  void SetSidebarShowOption(int show_options);

  SidebarService(const SidebarService&) = delete;
  SidebarService& operator=(const SidebarService&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(SidebarServiceTest, AddRemoveItems);

  void LoadSidebarItems();
  void UpdateSidebarItemsToPrefStore();
  std::vector<SidebarItem> GetDefaultSidebarItemsFromCurrentItems() const;
  void OnPreferenceChanged(const std::string& pref_name);

  PrefService* prefs_ = nullptr;
  std::vector<SidebarItem> items_;
  base::ObserverList<Observer> observers_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_SERVICE_H_
