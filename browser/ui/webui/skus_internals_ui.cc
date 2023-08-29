// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/skus_internals_ui.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "brave/browser/brave_browser_process.h"
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
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"
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
    // Only shows "skus:xx" kv in webui.
    if (!base::StartsWith(kv.first, "skus:")) {
      continue;
    }

    if (auto value = base::JSONReader::Read(kv.second.GetString()); value) {
      dict.Set(kv.first, std::move(*value));
    }
  }

  std::string result;
  base::JSONWriter::Write(dict, &result);
  std::move(callback).Run(result);
}

void SkusInternalsUI::GetVpnState(GetVpnStateCallback callback) {
  base::Value::Dict dict;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
#if !BUILDFLAG(IS_ANDROID)
  dict.Set("Last connection error", GetLastVPNConnectionError());
#endif
  dict.Set("Order", GetVPNOrderInfo());
#endif
  std::string result;
  base::JSONWriter::Write(dict, &result);
  std::move(callback).Run(result);
}

base::Value::Dict SkusInternalsUI::GetVPNOrderInfo() const {
  base::Value::Dict dict;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* profile = Profile::FromWebUI(web_ui());
  if (!brave_vpn::IsBraveVPNEnabled(profile->GetPrefs())) {
    return dict;
  }

  dict.Set("env",
           local_state_->GetString(brave_vpn::prefs::kBraveVPNEnvironment));

  const auto& skus_state = local_state_->GetDict(skus::prefs::kSkusState);
  for (const auto kv : skus_state) {
    if (!base::StartsWith(kv.first, "skus:")) {
      continue;
    }

    // Convert to Value as it's stored as string in local state.
    auto json_value = base::JSONReader::Read(kv.second.GetString());
    if (!json_value) {
      continue;
    }

    const auto* skus = json_value->GetIfDict();
    if (!skus) {
      continue;
    }

    const auto* orders = skus->FindDict("orders");
    if (!orders) {
      continue;
    }
    base::Value::Dict order_dict_output;
    for (const auto order : *orders) {
      const auto* order_dict = order.second.GetIfDict();
      if (!order_dict) {
        continue;
      }

      if (auto* location = order_dict->FindString("location")) {
        if (!base::StartsWith(*location, "vpn.")) {
          continue;
        }
        order_dict_output.Set("location", *location);
      }

      if (auto* id = order_dict->FindString("id")) {
        order_dict_output.Set("id", *id);
      }
      if (auto* expires_at = order_dict->FindString("expires_at")) {
        order_dict_output.Set("expires_at", *expires_at);
      }
    }
    // Set output with env like {skus:production: {...}}.
    dict.Set(kv.first, std::move(order_dict_output));
  }
#endif
  return dict;
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

std::string SkusInternalsUI::GetLastVPNConnectionError() const {
  std::string error;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* api = g_brave_browser_process->brave_vpn_os_connection_api();
  CHECK(api);
  error = api->GetLastConnectionError();
#endif
  return error;
}
WEB_UI_CONTROLLER_TYPE_IMPL(SkusInternalsUI)
