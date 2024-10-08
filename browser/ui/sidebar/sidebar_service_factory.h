/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_SERVICE_FACTORY_H_

#include <memory>
#include <vector>

#include "brave/components/sidebar/browser/sidebar_item.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace sidebar {

class SidebarService;

class SidebarServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static SidebarServiceFactory* GetInstance();
  static SidebarService* GetForProfile(Profile* profile);

 private:
  friend base::NoDestructor<SidebarServiceFactory>;
  friend class SidebarBrowserTest;

  // This is the default display order
  static constexpr SidebarItem::BuiltInItemType kDefaultBuiltInItemTypes[] = {
      SidebarItem::BuiltInItemType::kChatUI,
      SidebarItem::BuiltInItemType::kBraveTalk,
      SidebarItem::BuiltInItemType::kWallet,
      SidebarItem::BuiltInItemType::kBookmarks,
      SidebarItem::BuiltInItemType::kReadingList,
      SidebarItem::BuiltInItemType::kHistory,
      SidebarItem::BuiltInItemType::kPlaylist};
  static_assert(
      std::size(kDefaultBuiltInItemTypes) ==
          static_cast<size_t>(SidebarItem::BuiltInItemType::kBuiltInItemLast),
      "A built-in item in this visual order is missing or you might forget to "
      "update kBuiltInItemItemLast value. If you want to add a "
      "new item while keeping that hidden, please visit "
      "GetBuiltInItemForType() in sidebar_service.cc");

  SidebarServiceFactory();
  ~SidebarServiceFactory() override;

  SidebarServiceFactory(const SidebarServiceFactory&) = delete;
  SidebarServiceFactory& operator=(const SidebarServiceFactory&) = delete;

  std::vector<SidebarItem::BuiltInItemType> GetBuiltInItemTypesForProfile(
      Profile* profile) const;

  // BrowserContextKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_SERVICE_FACTORY_H_
