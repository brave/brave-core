// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/skus_internals_ui.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/values.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/resources/grit/skus_internals_generated_map.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/browser/brave_vpn_service_helper.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

SkusInternalsUI::SkusInternalsUI(content::WebUI* web_ui,
                                 const std::string& name)
    : content::WebUIController(web_ui),
      local_state_(g_browser_process->local_state()) {
  CreateAndAddWebUIDataSource(web_ui, name, kSkusInternalsGenerated,
                              kSkusInternalsGeneratedSize,
                              IDR_SKUS_INTERNALS_HTML);
}

SkusInternalsUI::~SkusInternalsUI() = default;

void SkusInternalsUI::BindInterface(
    mojo::PendingReceiver<skus::mojom::SkusInternals> pending_receiver) {
  if (skus_internals_receiver_.is_bound()) {
    skus_internals_receiver_.reset();
  }

  skus_internals_receiver_.Bind(std::move(pending_receiver));
}

void SkusInternalsUI::GetEventLog(GetEventLogCallback callback) {
  // TODO(simonhong): Ask log to SkusService
  NOTIMPLEMENTED_LOG_ONCE();
}

void SkusInternalsUI::GetSkusState(GetSkusStateCallback callback) {
  // TODO(simonhong): Determine which value should be displayed.
  const auto& skus_state = local_state_->GetDict(skus::prefs::kSkusState);
  base::Value::Dict dict;

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* profile = Profile::FromWebUI(web_ui());
  if (brave_vpn::IsBraveVPNEnabled(profile->GetPrefs())) {
    dict.Set("env",
             local_state_->GetString(brave_vpn::prefs::kBraveVPNEnvironment));
  }
#endif

  for (const auto kv : skus_state) {
    if (auto value = base::JSONReader::Read(kv.second.GetString()); value) {
      dict.Set(kv.first, std::move(*value));
    }
  }

  std::string result;
  base::JSONWriter::Write(dict, &result);
  std::move(callback).Run(result);
}

void SkusInternalsUI::ResetSkusState() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  // VPN service caches credentials. It should be cleared also
  // when skus state is reset. Otherwise, vpn service is still
  // in purchased state.
  auto* profile = Profile::FromWebUI(web_ui());
  if (brave_vpn::IsBraveVPNEnabled(profile->GetPrefs())) {
    brave_vpn::ClearSubscriberCredential(local_state_);
  }
#endif

  local_state_->ClearPref(skus::prefs::kSkusState);
}

WEB_UI_CONTROLLER_TYPE_IMPL(SkusInternalsUI)
