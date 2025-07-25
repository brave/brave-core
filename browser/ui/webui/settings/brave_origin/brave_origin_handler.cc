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

  // TODO(bsclifton): OR this against the other two
  // waiting for https://github.com/brave/brave-core/pull/30022
  initial_state.Set("p3a_stats_crash",
                    g_browser_process->local_state()->GetBoolean(
                        metrics::prefs::kMetricsReportingEnabled));

  // TODO(bsclifton): implement the rest.
  // search ads
  // email alias
  // sidebar
  // web3domains

  ResolveJavascriptCallback(args[0], initial_state);
}

void BraveOriginHandler::HandleToggleValue(const base::Value::List& args) {
  CHECK_EQ(args.size(), 2U);
  std::string pref_name = args[0].GetString();
  bool enabled = args[1].GetBool();

  if (pref_name == "p3a_stats_crash") {
    std::vector<std::string> p3a_stats_crash_prefs = {
        // TODO (bsclifton): add 2 preferences from here:
        // https://github.com/brave/brave-core/pull/30022
        metrics::prefs::kMetricsReportingEnabled};
    for (const std::string& sub_pref : p3a_stats_crash_prefs) {
      g_browser_process->local_state()->SetBoolean(sub_pref, enabled);
    }
    return;
  }

  static constexpr auto inverted_local_state =
      base::MakeFixedFlatMap<std::string_view, std::string_view>({
#if BUILDFLAG(ENABLE_TOR)
          {"tor", tor::prefs::kTorDisabled}
#endif
      });

  const auto it = inverted_local_state.find(pref_name);
  if (it != inverted_local_state.end()) {
    LOG(ERROR) << "BSC]] local:" << pref_name << " | " << enabled;
    g_browser_process->local_state()->SetBoolean(it->second, !enabled);
    AllowJavascript();
    return;
  }

  // handle regular ones here
  LOG(ERROR) << "BSC]] NOT FOUND: " << pref_name;
}

void BraveOriginHandler::HandleResetToDefaults(const base::Value::List& args) {
  LOG(ERROR) << "BSC]] HandleResetToDefaults";
}

void BraveOriginHandler::OnValueChanged(const std::string& pref_name) {
  if (IsJavascriptAllowed()) {
    static constexpr auto inverted_local_state =
        base::MakeFixedFlatMap<std::string_view, std::string_view>({
#if BUILDFLAG(ENABLE_TOR)
            {tor::prefs::kTorDisabled, "tor-enabled-changed"},
#endif
        });

    const auto it = inverted_local_state.find(pref_name);
    if (it != inverted_local_state.end()) {
      LOG(ERROR) << "BSC]] local_state; " << it->first << " | " << it->second;
      FireWebUIListener(
          it->second, base::Value(!g_browser_process->local_state()->GetBoolean(
                          it->first)));
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

  bool rewards_changed =
      was_rewards_enabled_ ==
      prefs->GetBoolean(brave_rewards::prefs::kDisabledByPolicy);
  bool ai_changed =
      was_ai_enabled_ != prefs->GetBoolean(ai_chat::prefs::kEnabledByPolicy);
  bool news_changed =
      was_news_enabled_ ==
      prefs->GetBoolean(brave_news::prefs::kBraveNewsDisabledByPolicy);
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

  // TODO: put the P3A check here too
  if (rewards_changed || ai_changed || news_changed || vpn_changed ||
      tor_changed || wallet_changed) {
    return true;
  }
  return false;
}
