// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar_handler.h"

#include "brave/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/brave_action.h"
#include "brave/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/list_action_modifiers.h"

#define ListActions ListActionsChromium
#define PinAction PinActionChromium

#include "src/chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar_handler.cc"

#undef PinAction
#undef ListActions

void CustomizeToolbarHandler::ListActions(ListActionsCallback callback) {
  ListActionsChromium(
      base::BindOnce(&customize_chrome::FilterUnsupportedChromiumActions)
          .Then(
              base::BindOnce(&customize_chrome::ApplyBraveSpecificModifications,
                             web_contents_->GetWeakPtr()))
          .Then(std::move(callback)));
}

void CustomizeToolbarHandler::PinAction(
    side_panel::customize_chrome::mojom::ActionId action_id,
    bool pin) {
  for (const auto& brave_action : customize_chrome::kBraveActions) {
    if (action_id == brave_action.id) {
      // Brave specific actions are handled here.
      prefs()->SetBoolean(brave_action.pref_name,
                          !prefs()->GetBoolean(brave_action.pref_name));
      return;
    }
  }

  PinActionChromium(action_id, pin);
}
