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
#include "chrome/grit/branded_strings.h"
#include "components/grit/brave_components_strings.h"

#define ListCategories ListCategories_ChromiumImpl
#define ListActions ListActions_ChromiumImpl
#define PinAction PinAction_ChromiumImpl

// pref_change_registrar_.Init() in constructor
#define Init(...)    \
  Init(__VA_ARGS__); \
  ObserveBraveActions()

// Replace the resource ID for the "Your Chrome" category with "Brave Menu"
// resource ID.
#undef IDS_NTP_CUSTOMIZE_TOOLBAR_CATEGORY_YOUR_CHROME
#define IDS_NTP_CUSTOMIZE_TOOLBAR_CATEGORY_YOUR_CHROME \
  IDS_CUSTOMIZE_TOOLBAR_CATEGORY_TOOLBAR

#include <chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar_handler.cc>

#undef IDS_NTP_CUSTOMIZE_TOOLBAR_CATEGORY_YOUR_CHROME

#undef Init
#undef PinAction
#undef ListActions
#undef ListCategories

void CustomizeToolbarHandler::ListCategories(ListCategoriesCallback callback) {
  ListCategories_ChromiumImpl(
      base::BindOnce(
          &customize_chrome::AppendBraveSpecificCategories,
          base::Unretained(base::raw_ref<content::WebContents>(*web_contents_)))
          .Then(std::move(callback)));
}

void CustomizeToolbarHandler::ListActions(ListActionsCallback callback) {
  ListActions_ChromiumImpl(
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

  PinAction_ChromiumImpl(action_id, pin);
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
