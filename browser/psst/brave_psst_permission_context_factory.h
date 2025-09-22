/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PSST_BRAVE_PSST_PERMISSION_CONTEXT_FACTORY_H_
#define BRAVE_BROWSER_PSST_BRAVE_PSST_PERMISSION_CONTEXT_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace psst {

class BravePsstPermissionContext;

class BravePsstPermissionContextFactory : public ProfileKeyedServiceFactory {
 public:
  static BravePsstPermissionContext* GetForProfile(Profile* profile);
  static BravePsstPermissionContextFactory* GetInstance();

  BravePsstPermissionContextFactory(const BravePsstPermissionContextFactory&) =
      delete;
  BravePsstPermissionContextFactory& operator=(
      const BravePsstPermissionContextFactory&) = delete;

 private:
  friend base::NoDestructor<BravePsstPermissionContextFactory>;

  BravePsstPermissionContextFactory();
  ~BravePsstPermissionContextFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_BRAVE_PSST_PERMISSION_CONTEXT_FACTORY_H_
