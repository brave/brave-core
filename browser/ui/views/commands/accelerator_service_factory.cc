// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/commands/accelerator_service_factory.h"

#include <utility>

#include "base/memory/singleton.h"
#include "brave/browser/ui/views/commands/accelerator_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/accelerator_table.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/accelerators/accelerator.h"

namespace commands {

// static
AcceleratorServiceFactory* AcceleratorServiceFactory::GetInstance() {
  return base::Singleton<AcceleratorServiceFactory>::get();
}

// static
AcceleratorService* AcceleratorServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<AcceleratorService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

AcceleratorServiceFactory::AcceleratorServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AcceleratorServiceFactory",
          BrowserContextDependencyManager::GetInstance()) {}

AcceleratorServiceFactory::~AcceleratorServiceFactory() = default;

KeyedService* AcceleratorServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  DCHECK(profile);

  Accelerators default_accelerators;
  for (const auto& accelerator_info : GetAcceleratorList()) {
    default_accelerators[accelerator_info.command_id].push_back(
        ui::Accelerator(accelerator_info.keycode, accelerator_info.modifiers));
  }
  return new AcceleratorService(profile->GetPrefs(),
                                std::move(default_accelerators));
}

content::BrowserContext* AcceleratorServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace commands
