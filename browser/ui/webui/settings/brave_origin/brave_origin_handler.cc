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
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

BraveOriginHandler::BraveOriginHandler(Profile* profile) : profile_(profile) {
  // Profile prefs
  pref_change_registrar_.Init(profile_->GetPrefs());
  pref_change_registrar_.Add(
      brave_rewards::prefs::kDisabledByPolicy,
      base::BindRepeating(&BraveOriginHandler::OnValueChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      ai_chat::prefs::kEnabledByPolicy,
      base::BindRepeating(&BraveOriginHandler::OnValueChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_news::prefs::kBraveNewsDisabledByPolicy,
      base::BindRepeating(&BraveOriginHandler::OnValueChanged,
                          base::Unretained(this)));
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  pref_change_registrar_.Add(
      brave_vpn::prefs::kManagedBraveVPNDisabled,
      base::BindRepeating(&BraveOriginHandler::OnValueChanged,
                          base::Unretained(this)));
#endif
  pref_change_registrar_.Add(
      brave_wallet::prefs::kDisabledByPolicy,
      base::BindRepeating(&BraveOriginHandler::OnValueChanged,
                          base::Unretained(this)));

  // Local state (spans all profiles)
  local_state_change_registrar_.Init(g_browser_process->local_state());
  local_state_change_registrar_.Add(
      p3a::kP3ADisabledByPolicy,
      base::BindRepeating(&BraveOriginHandler::OnValueChanged,
                          base::Unretained(this)));
  local_state_change_registrar_.Add(
      kStatsReportingDisabledByPolicy,
      base::BindRepeating(&BraveOriginHandler::OnValueChanged,
                          base::Unretained(this)));
  local_state_change_registrar_.Add(
      metrics::prefs::kMetricsReportingEnabled,
      base::BindRepeating(&BraveOriginHandler::OnValueChanged,
                          base::Unretained(this)));
#if BUILDFLAG(ENABLE_TOR)
  local_state_change_registrar_.Add(
      tor::prefs::kTorDisabled,
      base::BindRepeating(&BraveOriginHandler::OnValueChanged,
                          base::Unretained(this)));
#endif

  // Store value at time webui is loaded.
  // If the value changes we can show a restart toast.
  auto* prefs = profile_->GetPrefs();
  was_rewards_enabled_ =
      !prefs->GetBoolean(brave_rewards::prefs::kDisabledByPolicy);
  was_ai_enabled_ = !prefs->GetBoolean(ai_chat::prefs::kEnabledByPolicy);
  was_news_enabled_ =
      !prefs->GetBoolean(brave_news::prefs::kBraveNewsDisabledByPolicy);
  was_p3a_enabled_ =
      !g_browser_process->local_state()->GetBoolean(p3a::kP3ADisabledByPolicy);
  was_stats_reporting_enabled_ = !g_browser_process->local_state()->GetBoolean(
      kStatsReportingDisabledByPolicy);
  was_crash_reporting_enabled_ = g_browser_process->local_state()->GetBoolean(
      metrics::prefs::kMetricsReportingEnabled);
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  was_vpn_enabled_ =
      !prefs->GetBoolean(brave_vpn::prefs::kManagedBraveVPNDisabled);
#endif
#if BUILDFLAG(ENABLE_TOR)
  was_tor_enabled_ =
      !g_browser_process->local_state()->GetBoolean(tor::prefs::kTorDisabled);
#endif
  was_wallet_enabled_ =
      !prefs->GetBoolean(brave_wallet::prefs::kDisabledByPolicy);
}

BraveOriginHandler::~BraveOriginHandler() = default;

void BraveOriginHandler::RegisterMessages() {
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
}

void BraveOriginHandler::HandleGetInitialState(const base::Value::List& args) {
  AllowJavascript();

  base::Value::Dict initial_state;
  initial_state.Set("enabled",
                    BraveOriginState::GetInstance()->IsBraveOriginUser());

  // We only need to capture values which are not accessible by preference.
  // Note that any preference bound will need an entry in:
  // `brave/browser/extensions/api/settings_private/brave_prefs_util.cc`
  // in order to be picked up in the polymer code.

#if BUILDFLAG(ENABLE_TOR)
  bool tor_disabled =
      g_browser_process->local_state()->GetBoolean(tor::prefs::kTorDisabled);
  initial_state.Set("tor", !tor_disabled);
  if (g_browser_process->local_state()->IsManagedPreference(
          tor::prefs::kTorDisabled)) {
    initial_state.Set("torManaged", true);
  }
#endif

  bool p3a_disabled =
      g_browser_process->local_state()->GetBoolean(p3a::kP3ADisabledByPolicy);
  initial_state.Set("p3a", !p3a_disabled);
  if (g_browser_process->local_state()->IsManagedPreference(
          p3a::kP3ADisabledByPolicy)) {
    initial_state.Set("p3aManaged", true);
  }

  bool stats_reporting_disabled = g_browser_process->local_state()->GetBoolean(
      kStatsReportingDisabledByPolicy);
  initial_state.Set("statsReporting", !stats_reporting_disabled);
  if (g_browser_process->local_state()->IsManagedPreference(
          kStatsReportingDisabledByPolicy)) {
    initial_state.Set("statsReportingManaged", true);
  }

  initial_state.Set("crashReporting",
                    g_browser_process->local_state()->GetBoolean(
                        metrics::prefs::kMetricsReportingEnabled));
  if (g_browser_process->local_state()->IsManagedPreference(
          metrics::prefs::kMetricsReportingEnabled)) {
    initial_state.Set("crashReportingManaged", true);
  }

  // TODO(bsclifton): implement others:
  // search ads
  // email alias | https://github.com/brave/brave-core/pull/27127
  // sidebar

  ResolveJavascriptCallback(args[0], initial_state);
}

void BraveOriginHandler::HandleToggleValue(const base::Value::List& args) {
  CHECK_EQ(args.size(), 2U);
  std::string pref_name = args[0].GetString();
  bool enabled = args[1].GetBool();

  AllowJavascript();

  // Although SettingsBooleanControlMixin  supports `inverted`, this does not
  // work as expected for custom prefs; we need to track manually.
  //
  // Ideally, we would bind local_state along with profile prefs.
  // I don't think it's possible to bind both.
  static constexpr auto inverted_local_state =
      base::MakeFixedFlatMap<std::string_view, std::string_view>({
#if BUILDFLAG(ENABLE_TOR)
          {"tor", tor::prefs::kTorDisabled},
#endif
          {"p3a", p3a::kP3ADisabledByPolicy},
          {"statsReporting", kStatsReportingDisabledByPolicy}});
  static constexpr auto local_state =
      base::MakeFixedFlatMap<std::string_view, std::string_view>(
          {{// TODO(bsclifton):
            // for some reason `metrics::prefs::kMetricsReportingEnabled`
            // is not constexpr. hardcoding the name for now.
            "crashReporting", "user_experience_metrics.reporting_enabled"}});

  const auto it_inverted = inverted_local_state.find(pref_name);
  if (it_inverted != inverted_local_state.end()) {
    g_browser_process->local_state()->SetBoolean(it_inverted->second, !enabled);
    return;
  }

  const auto it = local_state.find(pref_name);
  if (it != local_state.end()) {
    g_browser_process->local_state()->SetBoolean(it->second, enabled);
    return;
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
  if (IsJavascriptAllowed()) {
    static constexpr auto inverted_local_state =
        base::MakeFixedFlatMap<std::string_view, std::string_view>({
#if BUILDFLAG(ENABLE_TOR)
            {tor::prefs::kTorDisabled, "tor-enabled-changed"},
#endif
            {p3a::kP3ADisabledByPolicy, "p3a-enabled-changed"},
            {kStatsReportingDisabledByPolicy,
             "statsReporting-enabled-changed"}});
    static constexpr auto local_state =
        base::MakeFixedFlatMap<std::string_view, std::string_view>(
            {{// TODO(bsclifton):
              // for some reason `metrics::prefs::kMetricsReportingEnabled`
              // is not constexpr. hardcoding the name for now.
              "user_experience_metrics.reporting_enabled",
              "crashReporting-enabled-changed"}});

    const auto it_inverted = inverted_local_state.find(pref_name);
    if (it_inverted != inverted_local_state.end()) {
      FireWebUIListener(
          it_inverted->second,
          base::Value(!g_browser_process->local_state()->GetBoolean(
              it_inverted->first)));
      OnRestartNeededChanged();
      return;
    }

    const auto it = local_state.find(pref_name);
    if (it != local_state.end()) {
      FireWebUIListener(
          it->second,
          base::Value(g_browser_process->local_state()->GetBoolean(it->first)));
      OnRestartNeededChanged();
      return;
    }

    OnRestartNeededChanged();
  }
}

void BraveOriginHandler::OnRestartNeededChanged() {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("brave-needs-restart-changed",
                      base::Value(IsRestartNeeded()));
  }
}

bool BraveOriginHandler::IsRestartNeeded() {
  auto* prefs = profile_->GetPrefs();

  // TODO(bsclifton): clean this up. this is horrible and i feel bad.
  bool rewards_changed =
      was_rewards_enabled_ ==
      prefs->GetBoolean(brave_rewards::prefs::kDisabledByPolicy);
  bool ai_changed =
      was_ai_enabled_ != prefs->GetBoolean(ai_chat::prefs::kEnabledByPolicy);
  bool news_changed =
      was_news_enabled_ ==
      prefs->GetBoolean(brave_news::prefs::kBraveNewsDisabledByPolicy);
  bool p3a_changed =
      was_p3a_enabled_ ==
      g_browser_process->local_state()->GetBoolean(p3a::kP3ADisabledByPolicy);
  bool stats_reporting_changed = was_stats_reporting_enabled_ ==
                                 g_browser_process->local_state()->GetBoolean(
                                     kStatsReportingDisabledByPolicy);
  bool crash_reporting_changed = was_crash_reporting_enabled_ !=
                                 g_browser_process->local_state()->GetBoolean(
                                     metrics::prefs::kMetricsReportingEnabled);
  bool vpn_changed =
#if BUILDFLAG(ENABLE_BRAVE_VPN)
      was_vpn_enabled_ ==
      prefs->GetBoolean(brave_vpn::prefs::kManagedBraveVPNDisabled);
#else
      false;
#endif
  bool tor_changed =
#if BUILDFLAG(ENABLE_TOR)
      was_tor_enabled_ ==
      g_browser_process->local_state()->GetBoolean(tor::prefs::kTorDisabled);
#else
      false;
#endif
  bool wallet_changed =
      was_wallet_enabled_ ==
      prefs->GetBoolean(brave_wallet::prefs::kDisabledByPolicy);

  if (rewards_changed || ai_changed || news_changed || p3a_changed ||
      stats_reporting_changed || crash_reporting_changed || vpn_changed ||
      tor_changed || wallet_changed) {
    return true;
  }
  return false;
}
