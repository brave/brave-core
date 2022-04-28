// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_ACTION_VIEW_CONTROLLER_FACTORY_H_
#define BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_ACTION_VIEW_CONTROLLER_FACTORY_H_

#include <memory>

#include "brave/browser/ui/brave_actions/brave_action_view_controller.h"
#include "extensions/common/extension_id.h"

class Browser;
class ExtensionsContainer;

class BraveActionViewControllerFactory {
 public:
  static std::unique_ptr<BraveActionViewController> Create(
      const extensions::ExtensionId& extension_id,
      Browser* browser,
      ExtensionsContainer* extensions_container);
};

#endif  // BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_ACTION_VIEW_CONTROLLER_FACTORY_H_
