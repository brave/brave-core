// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace ipfs::ipld {

class BlockOrchestratorService;

class BlockOrchestratorServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static BlockOrchestratorService* GetServiceForContext(
      content::BrowserContext* context);
  static BlockOrchestratorServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<BlockOrchestratorServiceFactory>;

  BlockOrchestratorServiceFactory();
  ~BlockOrchestratorServiceFactory() override;

  BlockOrchestratorServiceFactory(const BlockOrchestratorServiceFactory&) =
      delete;
  BlockOrchestratorServiceFactory& operator=(
      const BlockOrchestratorServiceFactory&) = delete;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace ipfs::ipld
