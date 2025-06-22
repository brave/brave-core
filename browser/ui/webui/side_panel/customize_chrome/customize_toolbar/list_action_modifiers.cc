// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/list_action_modifiers.h"

#include <vector>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"

namespace customize_chrome {

using side_panel::customize_chrome::mojom::ActionId;
using side_panel::customize_chrome::mojom::ActionPtr;

std::vector<ActionPtr> FilterUnsupportedChromiumActions(
    std::vector<ActionPtr> actions) {
  // Filter out unsupported Chromium actions.
  std::erase_if(actions, [](const ActionPtr& action) -> bool {
    static constexpr auto kUnsupportedChromiumActions =
        base::MakeFixedFlatSet<ActionId>(
            {ActionId::kShowPaymentMethods, ActionId::kShowTranslate,
             ActionId::kShowReadAnything, ActionId::kShowAddresses});
    return base::Contains(kUnsupportedChromiumActions, action->id);
  });
  return actions;
}

}  // namespace customize_chrome
