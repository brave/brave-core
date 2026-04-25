/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

class SharedPinnedTabService;

class SharedPinnedTabServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static SharedPinnedTabService* GetForProfile(Profile* profile);

  static SharedPinnedTabServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<SharedPinnedTabServiceFactory>;

  SharedPinnedTabServiceFactory();
  ~SharedPinnedTabServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

#endif  // BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_SERVICE_FACTORY_H_
