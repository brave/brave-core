// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/list_action_modifiers.h"

#include <vector>

namespace customize_chrome {

using side_panel::customize_chrome::mojom::ActionId;
using side_panel::customize_chrome::mojom::ActionPtr;

std::vector<ActionPtr> FilterUnsupportedChromiumActions(
    std::vector<ActionPtr> actions) {
  // Filter out unsupported Chromium actions.
  std::erase_if(actions, [](const ActionPtr& action) -> bool {
    return action->id == ActionId::kShowPaymentMethods ||
           action->id == ActionId::kShowTranslate ||
           action->id == ActionId::kShowReadAnything ||
           action->id == ActionId::kShowAddresses;
  });
  return actions;
}

}  // namespace customize_chrome
