/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_origin/brave_origin_handler.h"

#include <memory>

#include "base/check.h"
#include "base/command_line.h"
#include "base/containers/fixed_flat_map.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/task/thread_pool.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_origin/brave_origin_state.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

namespace {

// TODO(https://github.com/brave/brave-browser/issues/48157)
// remove once //components/metrics/metrics_pref_names.h
// converts the constants to a constexpr.
inline constexpr char kMetricsReportingEnabled[] =
    "user_experience_metrics.reporting_enabled";

// Represents a profile preference or local state preference.
struct PrefConfig {
  std::string_view pref_name;
  // Many of the preferences being used here are "___DisabledByPolicy".
  // Because the UI is showing a toggle with true meaning enabled,
  // those negative values need to be inverted.
  bool inverted;
  std::string_view ui_key;
  std::string_view change_event;
};

// Profile preferences configuration
constexpr PrefConfig kProfilePrefs[] = {
    {brave_rewards::prefs::kDisabledByPolicy, true, "rewards",
     "rewards-enabled-changed"},
    {ai_chat::prefs::kEnabledByPolicy, false, "ai", "ai-enabled-changed"},
    {brave_news::prefs::kBraveNewsDisabledByPolicy, true, "news",
     "news-enabled-changed"},
    {kBraveTalkDisabledByPolicy, true, "talk", "talk-enabled-changed"},
#if BUILDFLAG(ENABLE_SPEEDREADER)
    {speedreader::kSpeedreaderPrefFeatureEnabled, false, "speedreader",
     "speedreader-enabled-changed"},
#endif
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
    {kBraveWaybackMachineEnabled, false, "wayback", "wayback-enabled-changed"},
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    {brave_vpn::prefs::kManagedBraveVPNDisabled, true, "vpn",
     "vpn-enabled-changed"},
#endif
    {brave_wallet::prefs::kDisabledByPolicy, true, "wallet",
     "wallet-enabled-changed"},
    {kWebDiscoveryEnabled, false, "webDiscovery",
     "web-discovery-enabled-changed"},
};

// Local state preferences configuration
constexpr PrefConfig kLocalStatePrefs[] = {
    {p3a::kP3AEnabled, false, "p3a", "p3a-enabled-changed"},
    {kStatsReportingEnabled, false, "statsReporting",
     "statsReporting-enabled-changed"},
    {kMetricsReportingEnabled, false, "crashReporting",
     "crashReporting-enabled-changed"},
#if BUILDFLAG(ENABLE_TOR)
    {tor::prefs::kTorDisabled, true, "tor", "tor-enabled-changed"},
#endif
};

// Map for handling toggle operations
constexpr auto kToggleLocalStateMap =
    base::MakeFixedFlatMap<std::string_view, std::string_view>({
#if BUILDFLAG(ENABLE_TOR)
        {"tor", tor::prefs::kTorDisabled},
#endif
        {"p3a", p3a::kP3AEnabled},
        {"statsReporting", kStatsReportingEnabled},
        {"crashReporting", kMetricsReportingEnabled}});

// Helper function to get preference value with inversion handling
bool GetPrefValue(PrefService* prefs,
                  std::string_view pref_name,
                  bool inverted) {
  bool value = prefs->GetBoolean(pref_name);
  bool actualValue = inverted ? !value : value;
  return actualValue;
}

// Helper function to set preference value with inversion handling
void SetPrefValue(PrefService* prefs,
                  std::string_view pref_name,
                  bool enabled,
                  bool inverted) {
  bool value = inverted ? !enabled : enabled;
  g_browser_process->local_state()->SetBoolean(pref_name, value);
}

}  // namespace

BraveOriginHandler::BraveOriginHandler() = default;
BraveOriginHandler::~BraveOriginHandler() = default;

void BraveOriginHandler::StoreInitialValues() {
  auto* prefs = profile_->GetPrefs();
  auto* local_state = g_browser_process->local_state();

  // Store profile preference initial values
  for (const auto& config : kProfilePrefs) {
    initial_values_[config.pref_name] =
        GetPrefValue(prefs, config.pref_name, config.inverted);
  }

  // Store local state preference initial values
  for (const auto& config : kLocalStatePrefs) {
    initial_values_[config.pref_name] =
        GetPrefValue(local_state, config.pref_name, config.inverted);
  }
}

void BraveOriginHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());

  web_ui()->RegisterMessageCallback(
      "getInitialState",
      base::BindRepeating(&BraveOriginHandler::HandleGetInitialState,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "toggleValue", base::BindRepeating(&BraveOriginHandler::HandleToggleValue,
                                         base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "resetToDefaults",
      base::BindRepeating(&BraveOriginHandler::HandleResetToDefaults,
                          base::Unretained(this)));

  // Register profile preference change listeners
  pref_change_registrar_.Init(profile_->GetPrefs());
  for (const auto& config : kProfilePrefs) {
    pref_change_registrar_.Add(
        config.pref_name,
        base::BindRepeating(&BraveOriginHandler::OnValueChanged,
                            base::Unretained(this)));
  }

  // Register local state preference change listeners
  local_state_change_registrar_.Init(g_browser_process->local_state());
  for (const auto& config : kLocalStatePrefs) {
    local_state_change_registrar_.Add(
        config.pref_name,
        base::BindRepeating(&BraveOriginHandler::OnValueChanged,
                            base::Unretained(this)));
  }

  // Store initial values for restart detection
  StoreInitialValues();
}

// Make a dictionary w/ local state available to the UI.
// The UI has access to the profile preferences already (bound as `prefs`).
void BraveOriginHandler::HandleGetInitialState(const base::Value::List& args) {
  AllowJavascript();

  base::Value::Dict initial_state;
  initial_state.Set("enabled",
                    BraveOriginState::GetInstance()->IsBraveOriginUser());

  auto* local_state = g_browser_process->local_state();

  // Add local state preferences to initial state
  for (const auto& config : kLocalStatePrefs) {
    bool value = GetPrefValue(local_state, config.pref_name, config.inverted);
    initial_state.Set(config.ui_key, value);

    if (local_state->IsManagedPreference(config.pref_name)) {
      std::string managed_key = std::string(config.ui_key) + "Managed";
      initial_state.Set(managed_key, true);
    }
  }

  // TODO(https://github.com/brave/brave-browser/issues/48144)
  // implement others:
  //
  // email alias | https://github.com/brave/brave-core/pull/29700
  // sidebar

  ResolveJavascriptCallback(args[0], initial_state);
}

void BraveOriginHandler::HandleToggleValue(const base::Value::List& args) {
  CHECK_EQ(args.size(), 2U);
  std::string pref_name = args[0].GetString();
  bool enabled = args[1].GetBool();

  AllowJavascript();

  // Handle local state preferences
  const auto it = kToggleLocalStateMap.find(pref_name);
  if (it != kToggleLocalStateMap.end()) {
    // Find the config for this preference
    for (const auto& config : kLocalStatePrefs) {
      if (std::string_view(config.pref_name) == it->second) {
        SetPrefValue(g_browser_process->local_state(), config.pref_name,
                     enabled, config.inverted);
        return;
      }
    }
  }

  // handle regular ones here
  VLOG(2) << "Value not handled: " << pref_name;
}

void BraveOriginHandler::HandleResetToDefaults(const base::Value::List& args) {
  // TODO(https://github.com/brave/brave-browser/issues/47977)
  //
  // Maybe this can look something like:
  // BraveOriginState::GetInstance()->ResetSettingsToDefault();
}

void BraveOriginHandler::OnValueChanged(const std::string& pref_name) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  auto* local_state = g_browser_process->local_state();

  // Find and fire the appropriate change event
  for (const auto& config : kLocalStatePrefs) {
    if (config.pref_name == pref_name) {
      bool value = GetPrefValue(local_state, config.pref_name, config.inverted);
      FireWebUIListener(config.change_event, base::Value(value));
      OnRestartNeededChanged();
      return;
    }
  }

  OnRestartNeededChanged();
}

void BraveOriginHandler::OnRestartNeededChanged() {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("brave-needs-restart-changed",
                      base::Value(IsRestartNeeded()));
  }
}

bool BraveOriginHandler::IsRestartNeeded() {
  auto* prefs = profile_->GetPrefs();
  auto* local_state = g_browser_process->local_state();

  // Check if any preference has changed from its initial value
  for (const auto& config : kProfilePrefs) {
    auto it = initial_values_.find(config.pref_name);
    if (it != initial_values_.end()) {
      bool current_value =
          GetPrefValue(prefs, config.pref_name, config.inverted);
      if (it->second != current_value) {
        return true;
      }
    }
  }

  for (const auto& config : kLocalStatePrefs) {
    auto it = initial_values_.find(config.pref_name);
    if (it != initial_values_.end()) {
      bool current_value =
          GetPrefValue(local_state, config.pref_name, config.inverted);
      if (it->second != current_value) {
        return true;
      }
    }
  }

  return false;
}
