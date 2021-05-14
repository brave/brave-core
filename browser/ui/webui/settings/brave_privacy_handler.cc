/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"

#include <string>

#include "base/bind.h"
#include "base/values.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "third_party/blink/public/common/peerconnection/webrtc_ip_handling_policy.h"

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
#include "brave/browser/gcm_driver/brave_gcm_channel_status.h"
#endif

#if BUILDFLAG(BRAVE_P3A_ENABLED)
#include "brave/components/p3a/pref_names.h"
#endif

BravePrivacyHandler::BravePrivacyHandler() {
  local_state_change_registrar_.Init(g_browser_process->local_state());
#if BUILDFLAG(BRAVE_P3A_ENABLED)
  local_state_change_registrar_.Add(
      brave::kP3AEnabled,
      base::BindRepeating(&BravePrivacyHandler::OnP3AEnabledChanged,
                          base::Unretained(this)));
#endif
}

BravePrivacyHandler::~BravePrivacyHandler() {
  local_state_change_registrar_.RemoveAll();
}

void BravePrivacyHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());

#if BUILDFLAG(BRAVE_P3A_ENABLED)
  web_ui()->RegisterMessageCallback(
      "setP3AEnabled", base::BindRepeating(&BravePrivacyHandler::SetP3AEnabled,
                                           base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getP3AEnabled", base::BindRepeating(&BravePrivacyHandler::GetP3AEnabled,
                                           base::Unretained(this)));
#endif
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

#if BUILDFLAG(BRAVE_P3A_ENABLED)
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

void BravePrivacyHandler::OnP3AEnabledChanged() {
  if (IsJavascriptAllowed()) {
    PrefService* local_state = g_browser_process->local_state();
    bool enabled = local_state->GetBoolean(brave::kP3AEnabled);

    FireWebUIListener("p3a-enabled-changed", base::Value(enabled));
  }
}
#endif
