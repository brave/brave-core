/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_ONION_HOSTNAME_RESOLUTION_UPDATER_FACTORY_H_
#define BRAVE_BROWSER_TOR_ONION_HOSTNAME_RESOLUTION_UPDATER_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace tor {

// Creates an OnionHostnameResolutionUpdater for every regular, incognito and
// guest profile. Tor profiles always resolve .onion through Tor, so they get
// none.
class OnionHostnameResolutionUpdaterFactory
    : public ProfileKeyedServiceFactory {
 public:
  static OnionHostnameResolutionUpdaterFactory* GetInstance();

  OnionHostnameResolutionUpdaterFactory(
      const OnionHostnameResolutionUpdaterFactory&) = delete;
  OnionHostnameResolutionUpdaterFactory& operator=(
      const OnionHostnameResolutionUpdaterFactory&) = delete;

 private:
  friend base::NoDestructor<OnionHostnameResolutionUpdaterFactory>;

  OnionHostnameResolutionUpdaterFactory();
  ~OnionHostnameResolutionUpdaterFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_ONION_HOSTNAME_RESOLUTION_UPDATER_FACTORY_H_
