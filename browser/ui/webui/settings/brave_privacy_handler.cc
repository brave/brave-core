/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"

#include <string>

#include "base/bind.h"
#include "base/values.h"
#include "brave/components/p3a/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/webrtc_ip_handling_policy.h"
#include "content/public/browser/web_ui_data_source.h"

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
#include "brave/browser/gcm_driver/brave_gcm_channel_status.h"
#endif

void BravePrivacyHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());

  web_ui()->RegisterMessageCallback(
      "getWebRTCPolicy",
      base::BindRepeating(&BravePrivacyHandler::GetWebRTCPolicy,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setWebRTCPolicy",
      base::BindRepeating(&BravePrivacyHandler::SetWebRTCPolicy,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setP3AEnabled", base::BindRepeating(&BravePrivacyHandler::SetP3AEnabled,
                                           base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getP3AEnabled", base::BindRepeating(&BravePrivacyHandler::GetP3AEnabled,
                                           base::Unretained(this)));
}

// static
void BravePrivacyHandler::AddLoadTimeData(content::WebUIDataSource* data_source,
                                          Profile* profile) {
#if BUILDFLAG(USE_GCM_FROM_PLATFORM)
  data_source->AddBoolean("pushMessagingEnabledAtStartup",
                          true);
#else
  gcm::BraveGCMChannelStatus* gcm_channel_status =
      gcm::BraveGCMChannelStatus::GetForProfile(profile);

  DCHECK(gcm_channel_status);
  data_source->AddBoolean("pushMessagingEnabledAtStartup",
                          gcm_channel_status->IsGCMEnabled());
#endif
}

void BravePrivacyHandler::SetWebRTCPolicy(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);

  std::string policy;
  args->GetString(0, &policy);
  profile_->GetPrefs()->SetString(prefs::kWebRTCIPHandlingPolicy, policy);
}

void BravePrivacyHandler::GetWebRTCPolicy(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);

  std::string policy =
      profile_->GetPrefs()->GetString(prefs::kWebRTCIPHandlingPolicy);

  AllowJavascript();
  ResolveJavascriptCallback(args->GetList()[0].Clone(), base::Value(policy));
}

void BravePrivacyHandler::SetP3AEnabled(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);

  bool enabled;
  args->GetBoolean(0, &enabled);

  PrefService* local_state = g_browser_process->local_state();
  local_state->SetBoolean(brave::kP3AEnabled, enabled);
}

void BravePrivacyHandler::GetP3AEnabled(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);

  PrefService* local_state = g_browser_process->local_state();
  bool enabled = local_state->GetBoolean(brave::kP3AEnabled);

  AllowJavascript();
  ResolveJavascriptCallback(args->GetList()[0].Clone(), base::Value(enabled));
}
