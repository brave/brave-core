// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar_handler.h"

#include "base/containers/map_util.h"
#include "base/memory/raw_ref.h"
#include "base/notreached.h"
#include "brave/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/brave_action.h"
#include "brave/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/list_action_modifiers.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

#define ListActions ListActionsChromium
#define PinAction PinActionChromium

// pref_change_registrar_.Init() in constructor
#define Init(...)    \
  Init(__VA_ARGS__); \
  ObserveBraveActions()

#include "src/chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar_handler.cc"

#undef Init
#undef PinAction
#undef ListActions

void CustomizeToolbarHandler::ListActions(ListActionsCallback callback) {
  ListActionsChromium(
      base::BindOnce(&customize_chrome::FilterUnsupportedChromiumActions)
          .Then(base::BindOnce(
              &customize_chrome::ApplyBraveSpecificModifications,
              base::Unretained(
                  base::raw_ref<content::WebContents>(*web_contents_))))
          .Then(std::move(callback)));
}

void CustomizeToolbarHandler::PinAction(
    side_panel::customize_chrome::mojom::ActionId action_id,
    bool pin) {
  if (const auto* brave_action =
          base::FindPtrOrNull(customize_chrome::kBraveActions, action_id)) {
    // Brave specific actions are handled here.
    prefs()->SetBoolean(brave_action->pref_name,
                        !prefs()->GetBoolean(brave_action->pref_name));
    return;
  }

  PinActionChromium(action_id, pin);
}

void CustomizeToolbarHandler::ObserveBraveActions() {
  for (const auto& [id, brave_action] : customize_chrome::kBraveActions) {
    pref_change_registrar_.Add(
        brave_action->pref_name,
        base::BindRepeating(
            &CustomizeToolbarHandler::OnBraveActionPinnedChanged,
            base::Unretained(this), id));
  }
}

void CustomizeToolbarHandler::OnBraveActionPinnedChanged(
    side_panel::customize_chrome::mojom::ActionId action_id) {
  const auto* brave_action =
      base::FindPtrOrNull(customize_chrome::kBraveActions, action_id);
  CHECK(brave_action);
  client_->SetActionPinned(action_id,
                           prefs()->GetBoolean(brave_action->pref_name));
}
