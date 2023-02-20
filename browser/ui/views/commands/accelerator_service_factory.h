// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_COMMANDS_ACCELERATOR_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_UI_VIEWS_COMMANDS_ACCELERATOR_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/browser/ui/views/commands/accelerator_service.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace commands {

class AcceleratorServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static AcceleratorService* GetForContext(content::BrowserContext* context);
  static AcceleratorServiceFactory* GetInstance();

  AcceleratorServiceFactory(const AcceleratorServiceFactory&) = delete;
  AcceleratorServiceFactory& operator=(const AcceleratorServiceFactory&) =
      delete;

 private:
  friend struct base::DefaultSingletonTraits<AcceleratorServiceFactory>;

  AcceleratorServiceFactory();
  ~AcceleratorServiceFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace commands

#endif  // BRAVE_BROWSER_UI_VIEWS_COMMANDS_ACCELERATOR_SERVICE_FACTORY_H_
