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
#include "content/public/browser/browser_context.h"

namespace {

bool PinBraveAction(side_panel::customize_chrome::mojom::ActionId action_id,
                    PrefService* prefs,
                    bool pin) {
  if (const auto* brave_action =
          base::FindPtrOrNull(customize_chrome::kBraveActions, action_id)) {
    // Brave specific actions are handled here.
    auto* pref = prefs->FindPreference(brave_action->pref_name);
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
    // default value. This will help IsBraveActionCustomized() to detect if user
    // has set a customized value for Brave specific actions.
    if (is_default_value_is_pinned) {
      if (pin) {
        prefs->ClearPref(brave_action->pref_name);
      } else {
        prefs->SetBoolean(brave_action->pref_name, false);
      }
    } else {
      if (pin) {
        prefs->SetBoolean(brave_action->pref_name, true);
      } else {
        prefs->ClearPref(brave_action->pref_name);
      }
    }
    return true;
  }

  return false;
}

void OnBraveActionPinnedChanged(
    mojo::Remote<side_panel::customize_chrome::mojom::CustomizeToolbarClient>*
        client,
    PrefService* prefs,
    side_panel::customize_chrome::mojom::ActionId action_id) {
  const auto* brave_action =
      base::FindPtrOrNull(customize_chrome::kBraveActions, action_id);
  CHECK(brave_action);
  (*client)->SetActionPinned(action_id,
                             prefs->GetBoolean(brave_action->pref_name));
}

void ObserveBraveActions(
    mojo::Remote<side_panel::customize_chrome::mojom::CustomizeToolbarClient>*
        client,
    PrefChangeRegistrar& registrar) {
  for (const auto& [id, brave_action] : customize_chrome::kBraveActions) {
    // It's safe to bind unretained client and PrefService as they out lives
    // this registrar.
    registrar.Add(brave_action->pref_name,
                  base::BindRepeating(&OnBraveActionPinnedChanged,
                                      base::Unretained(client),
                                      base::Unretained(registrar.prefs()), id));
  }
}

bool IsBraveActionCustomized(content::WebContents* web_contents) {
  auto brave_actions = customize_chrome::ListBraveSpecificActions(web_contents);
  auto* prefs = user_prefs::UserPrefs::Get(web_contents->GetBrowserContext());
  CHECK(prefs) << "Browser context does not have prefs";
  return std::ranges::any_of(brave_actions, [prefs](const auto& action) {
    auto* pref = prefs->FindPreference(action.pref_name);
    return pref && !pref->IsDefaultValue();
  });
}

void ResetBraveActionsToDefault(content::WebContents* web_contents) {
  auto brave_actions = customize_chrome::ListBraveSpecificActions(web_contents);
  auto* prefs = user_prefs::UserPrefs::Get(web_contents->GetBrowserContext());
  CHECK(prefs) << "Browser context does not have prefs";

  for (const auto& action : brave_actions) {
    prefs->ClearPref(action.pref_name);
  }
}

}  // namespace

#include <chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar_handler.cc>
