/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_BRAVE_RENDERER_UPDATER_FACTORY_H_
#define BRAVE_BROWSER_PROFILES_BRAVE_RENDERER_UPDATER_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;
class BraveRendererUpdater;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

// Singleton that creates/deletes BraveRendererUpdater as new Profiles are
// created/shutdown.
class BraveRendererUpdaterFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns an instance of the BraveRendererUpdaterFactory singleton.
  static BraveRendererUpdaterFactory* GetInstance();

  // Returns the instance of RendererUpdater for the passed |profile|.
  static BraveRendererUpdater* GetForProfile(Profile* profile);

  BraveRendererUpdaterFactory(const BraveRendererUpdaterFactory&) = delete;
  BraveRendererUpdaterFactory& operator=(const BraveRendererUpdaterFactory&) =
      delete;

 protected:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* profile) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;

 private:
  friend base::NoDestructor<BraveRendererUpdaterFactory>;

  BraveRendererUpdaterFactory();
  ~BraveRendererUpdaterFactory() override;
};

#endif  // BRAVE_BROWSER_PROFILES_BRAVE_RENDERER_UPDATER_FACTORY_H_
