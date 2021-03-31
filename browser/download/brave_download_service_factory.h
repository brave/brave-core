/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/core/simple_keyed_service_factory.h"

class SimpleFactoryKey;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace download {
class DownloadService;
}  // namespace download

// BraveDownloadServiceFactory is the main client class for interaction with the
// download component.
class BraveDownloadServiceFactory : public SimpleKeyedServiceFactory {
 public:
  // Returns singleton instance of BraveDownloadServiceFactory.
  static BraveDownloadServiceFactory* GetInstance();

  // Returns the DownloadService associated with |key|.
  static download::DownloadService* GetForKey(SimpleFactoryKey* key);

 private:
  friend struct base::DefaultSingletonTraits<BraveDownloadServiceFactory>;

  BraveDownloadServiceFactory();
  ~BraveDownloadServiceFactory() override;

  // SimpleKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      SimpleFactoryKey* key) const override;
  SimpleFactoryKey* GetKeyToUse(SimpleFactoryKey* key) const override;

  DISALLOW_COPY_AND_ASSIGN(BraveDownloadServiceFactory);
};

#endif  // BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_SERVICE_FACTORY_H_
