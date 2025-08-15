/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/settings_private/brave_prefs_util.h"
#include "brave/components/brave_origin/brave_origin_state.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/settings_private.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_context.h"
#include "url/gurl.h"

namespace extensions {

std::optional<api::settings_private::PrefObject> BravePrefsUtil::GetPref(
    const std::string& name) {
  auto pref = PrefsUtil::GetPref(name);

  // Check if this preference is controlled by BraveOrigin
  auto* brave_origin_state = brave_origin::BraveOriginState::GetInstance();
  bool is_brave_origin_user =
      brave_origin_state && brave_origin_state->IsBraveOriginUser();

  if (pref && is_brave_origin_user) {
    if (brave_origin_state->IsPrefControlledByBraveOrigin(name)) {
      // Set a special extension ID to identify BraveOrigin controlled prefs
      // This is only meant to be used on the Brave Origin settings page.
      // Everywhere else we don't need to know if it is Brave Origin set
      // or not.
      pref->extension_id = "brave-origin";
    }
  }

  // Simulate "Enforced" mode for kCookieControlsMode pref when Cookies are
  // fully blocked via Shields. This will effectively disable the "Third-party
  // cookies" mode selector on Settings page.
  if (pref && name == prefs::kCookieControlsMode &&
      pref->enforcement == api::settings_private::Enforcement::kNone &&
      brave_shields::GetCookieControlType(
          HostContentSettingsMapFactory::GetForProfile(profile()),
          CookieSettingsFactory::GetForProfile(profile()).get(),
          GURL()) == brave_shields::ControlType::BLOCK) {
    pref->enforcement = api::settings_private::Enforcement::kEnforced;
  }
  return pref;
}

settings_private::SetPrefResult BravePrefsUtil::SetPref(
    const std::string& name,
    const base::Value* value) {
  // Check if this preference is controlled by BraveOrigin
  auto* brave_origin_state = brave_origin::BraveOriginState::GetInstance();
  bool is_brave_origin_user =
      brave_origin_state && brave_origin_state->IsBraveOriginUser();

  if (is_brave_origin_user &&
      brave_origin_state->IsPrefControlledByBraveOrigin(name)) {
    // For BraveOrigin-controlled preferences, store in policy settings
    auto* local_state = g_browser_process->local_state();
    // Get the profile policy service, not browser policy service
    auto* profile_policy_service =
        profile()->GetProfilePolicyConnector()->policy_service();

    if (value && local_state) {
      // Store the user's preference choice in kBraveOriginPolicySettings
      // The policy provider will read this and update the policy accordingly
      ScopedDictPrefUpdate update(
          local_state, brave_origin::prefs::kBraveOriginPolicySettings);
      update->Set(name, value->Clone());

      // Trigger policy refresh on the profile policy service
      // where the BraveOrigin policy provider is registered
      if (profile_policy_service) {
        profile_policy_service->RefreshPolicies(
            base::BindOnce([]() {}), policy::PolicyFetchReason::kUserRequest);
      }

      return settings_private::SetPrefResult::SUCCESS;
    }
    return settings_private::SetPrefResult::PREF_NOT_MODIFIABLE;
  }

  // For non-BraveOrigin preferences, use the default implementation
  return PrefsUtil::SetPref(name, value);
}

}  // namespace extensions
