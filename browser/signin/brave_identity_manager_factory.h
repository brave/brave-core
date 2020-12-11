/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SIGNIN_BRAVE_IDENTITY_MANAGER_FACTORY_H_
#define BRAVE_BROWSER_SIGNIN_BRAVE_IDENTITY_MANAGER_FACTORY_H_

#include "base/memory/singleton.h"
#include "chrome/browser/signin/identity_manager_factory.h"

namespace signin {
class BraveIdentityManager;
}

class Profile;

class BraveIdentityManagerFactory : public IdentityManagerFactory {
 public:
  BraveIdentityManagerFactory(const BraveIdentityManagerFactory&) = delete;
  BraveIdentityManagerFactory& operator=(const BraveIdentityManagerFactory&) =
      delete;

  // Returns an instance of the IdentityManagerFactory singleton.
  static BraveIdentityManagerFactory* GetInstance();

  static signin::BraveIdentityManager* GetForProfile(Profile* profile);
  static signin::BraveIdentityManager* GetForProfileIfExists(
      const Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<BraveIdentityManagerFactory>;
  BraveIdentityManagerFactory();
  ~BraveIdentityManagerFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

#endif  // BRAVE_BROWSER_SIGNIN_BRAVE_IDENTITY_MANAGER_FACTORY_H_
