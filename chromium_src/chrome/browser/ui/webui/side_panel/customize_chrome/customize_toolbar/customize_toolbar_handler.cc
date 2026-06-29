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
#include "chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/grit/branded_strings.h"
#include "components/grit/brave_components_strings.h"
#include "components/user_prefs/user_prefs.h"

#define ListCategories ListCategories_ChromiumImpl
#define ListActions ListActions_ChromiumImpl
#define PinAction PinAction_ChromiumImpl
#define GetIsCustomized GetIsCustomized_ChromiumImpl
#define ResetToDefault ResetToDefault_ChromiumImpl

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

#undef ResetToDefault
#undef GetIsCustomized
#undef Init
#undef PinAction
#undef ListActions
#undef ListCategories

void CustomizeToolbarHandler::ListCategories(ListCategoriesCallback callback) {
  ListCategories_ChromiumImpl(
      base::BindOnce(&customize_chrome::AppendBraveSpecificCategories,
                     web_contents_.get())
          .Then(std::move(callback)));
}

void CustomizeToolbarHandler::ListActions(ListActionsCallback callback) {
  if (!webui::GetBrowserWindowInterface(web_contents_)) {
    // This can happen if the web contents is shutting down. Upstream code is
    // has this check already.
    // https://github.com/brave/brave-browser/issues/53404
    std::move(callback).Run(
        std::vector<side_panel::customize_chrome::mojom::ActionPtr>());
    return;
  }

  ListActions_ChromiumImpl(
      base::BindOnce(&customize_chrome::FilterUnsupportedChromiumActions)
          .Then(
              base::BindOnce(&customize_chrome::ApplyBraveSpecificModifications,
                             web_contents_.get()))
          .Then(std::move(callback)));
}

void CustomizeToolbarHandler::PinAction(
    side_panel::customize_chrome::mojom::ActionId action_id,
    bool pin) {
  if (const auto* brave_action =
          base::FindPtrOrNull(customize_chrome::kBraveActions, action_id)) {
    // Brave specific actions are handled here.
    auto* pref = prefs()->FindPreference(brave_action->pref_name);
    CHECK(pref);

    bool is_default_value_is_pinned = false;
    if (pref->IsDefaultValue()) {
      is_default_value_is_pinned = pref->GetValue()->GetBool();
    } else {
      is_default_value_is_pinned = !pref->GetValue()->GetBool();
    }

    // In order to make it easy to detect if user has set a customized value for
    // Brave specific actions, we clear the pref when user want the state back
    // to default, instead of setting the pref with the same value as the
    // default value. This will help GetIsCustomized() to detect if user has set
    //  a customized value for Brave specific actions.
    if (is_default_value_is_pinned) {
      if (pin) {
        prefs()->ClearPref(brave_action->pref_name);
      } else {
        prefs()->SetBoolean(brave_action->pref_name, false);
      }
    } else {
      if (pin) {
        prefs()->SetBoolean(brave_action->pref_name, true);
      } else {
        prefs()->ClearPref(brave_action->pref_name);
      }
    }
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

void CustomizeToolbarHandler::GetIsCustomized(
    GetIsCustomizedCallback callback) {
  auto brave_actions =
      customize_chrome::ListBraveSpecificActions(web_contents_.get());
  auto* prefs = user_prefs::UserPrefs::Get(web_contents_->GetBrowserContext());
  CHECK(prefs) << "Browser context does not have prefs";
  if (std::ranges::any_of(brave_actions, [prefs](const auto& action) {
        auto* pref = prefs->FindPreference(action.pref_name);
        return pref && !pref->IsDefaultValue();
      })) {
    std::move(callback).Run(true);
    return;
  }

  GetIsCustomized_ChromiumImpl(std::move(callback));
}

void CustomizeToolbarHandler::ResetToDefault() {
  ResetToDefault_ChromiumImpl();

  auto brave_actions =
      customize_chrome::ListBraveSpecificActions(web_contents_.get());
  auto* prefs = user_prefs::UserPrefs::Get(web_contents_->GetBrowserContext());
  CHECK(prefs) << "Browser context does not have prefs";

  for (const auto& action : brave_actions) {
    prefs->ClearPref(action.pref_name);
  }
}
