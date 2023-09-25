/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_vpn/brave_vpn_handler.h"

#include <memory>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/task/thread_pool.h"
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"
#include "brave/components/brave_vpn/common/wireguard/win/storage_utils.h"
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/version_info.h"

namespace {

bool ElevatedRegisterBraveVPNService() {
  auto executable_path = brave_vpn::GetBraveVPNWireguardServiceExecutablePath();
  base::CommandLine cmd(executable_path);
  cmd.AppendSwitch(brave_vpn::kBraveVpnWireguardServiceInstallSwitchName);
  base::LaunchOptions options = base::LaunchOptions();
  options.wait = true;
  options.elevated = true;
  return base::LaunchProcess(cmd, options).IsValid();
}

}  // namespace

BraveVpnHandler::BraveVpnHandler(Profile* profile) : profile_(profile) {
  auto* service = brave_vpn::BraveVpnServiceFactory::GetForProfile(profile);
  CHECK(service);
  Observe(service);

  pref_change_registrar_.Init(g_browser_process->local_state());
  pref_change_registrar_.Add(
      brave_vpn::prefs::kBraveVPNWireguardEnabled,
      base::BindRepeating(&BraveVpnHandler::OnProtocolChanged,
                          base::Unretained(this)));
}

BraveVpnHandler::~BraveVpnHandler() = default;

void BraveVpnHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "registerWireguardService",
      base::BindRepeating(&BraveVpnHandler::HandleRegisterWireguardService,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isWireguardServiceRegistered",
      base::BindRepeating(&BraveVpnHandler::HandleIsWireguardServiceRegistered,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isBraveVpnConnected",
      base::BindRepeating(&BraveVpnHandler::HandleIsBraveVpnConnected,
                          base::Unretained(this)));
}

void BraveVpnHandler::OnProtocolChanged() {
  auto enabled =
      brave_vpn::IsBraveVPNWireguardEnabled(g_browser_process->local_state());
  brave_vpn::SetWireguardActive(enabled);
}

void BraveVpnHandler::HandleRegisterWireguardService(
    const base::Value::List& args) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&ElevatedRegisterBraveVPNService),
      base::BindOnce(&BraveVpnHandler::OnWireguardServiceRegistered,
                     weak_factory_.GetWeakPtr(), args[0].GetString()));
}

void BraveVpnHandler::OnWireguardServiceRegistered(
    const std::string& callback_id,
    bool success) {
  AllowJavascript();
  ResolveJavascriptCallback(callback_id, base::Value(success));
}

void BraveVpnHandler::HandleIsWireguardServiceRegistered(
    const base::Value::List& args) {
  AllowJavascript();

  ResolveJavascriptCallback(
      args[0],
      base::Value(brave_vpn::wireguard::IsWireguardServiceRegistered()));
}

void BraveVpnHandler::HandleIsBraveVpnConnected(const base::Value::List& args) {
  AllowJavascript();

  auto* service = brave_vpn::BraveVpnServiceFactory::GetForProfile(profile_);
  ResolveJavascriptCallback(args[0],
                            base::Value(service && service->IsConnected()));
}

void BraveVpnHandler::OnConnectionStateChanged(
    brave_vpn::mojom::ConnectionState state) {
  AllowJavascript();
  FireWebUIListener(
      "brave-vpn-state-change",
      base::Value(state == brave_vpn::mojom::ConnectionState::CONNECTED));
}

void BraveVpnHandler::OnJavascriptAllowed() {}

void BraveVpnHandler::OnJavascriptDisallowed() {}
