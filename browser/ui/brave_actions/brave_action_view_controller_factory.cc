// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/brave_actions/brave_action_view_controller_factory.h"

#include <utility>

#include "brave/browser/ui/brave_actions/brave_action_view_controller.h"
#include "brave/browser/ui/brave_actions/brave_rewards_action_view_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/extensions/extensions_container.h"
#include "extensions/browser/extension_action.h"
#include "extensions/browser/extension_action_manager.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"

// static
std::unique_ptr<BraveActionViewController>
BraveActionViewControllerFactory::Create(
    const extensions::ExtensionId& extension_id,
    Browser* browser,
    ExtensionsContainer* extensions_container) {
  DCHECK(browser);

  auto* registry = extensions::ExtensionRegistry::Get(browser->profile());
  scoped_refptr<const extensions::Extension> extension =
      registry->enabled_extensions().GetByID(extension_id);
  DCHECK(extension);
  extensions::ExtensionAction* extension_action =
      extensions::ExtensionActionManager::Get(browser->profile())
          ->GetExtensionAction(*extension);
  DCHECK(extension_action);

  // WrapUnique() because the constructors are private.
  if (extension_id == brave_rewards_extension_id) {
    return base::WrapUnique(new BraveRewardsActionViewController(
        std::move(extension), browser, extension_action, registry,
        extensions_container));
  } else {
    return base::WrapUnique(new BraveActionViewController(
        std::move(extension), browser, extension_action, registry,
        extensions_container));
  }
}
