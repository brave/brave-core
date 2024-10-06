/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_GREASELION_GREASELION_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_GREASELION_GREASELION_SERVICE_FACTORY_H_

#include <memory>

#include "base/files/file_path.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

class Profile;

namespace greaselion {

class GreaselionService;

class GreaselionServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  GreaselionServiceFactory(const GreaselionServiceFactory&) = delete;
  GreaselionServiceFactory& operator=(const GreaselionServiceFactory&) = delete;

  static GreaselionService* GetForBrowserContext(
      content::BrowserContext* context);
  static GreaselionServiceFactory* GetInstance();

  static base::FilePath GetInstallDirectory();

 private:
  friend base::NoDestructor<GreaselionServiceFactory>;

  GreaselionServiceFactory();
  ~GreaselionServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace greaselion

#endif  // BRAVE_BROWSER_GREASELION_GREASELION_SERVICE_FACTORY_H_
