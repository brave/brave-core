/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/interactive_main.h"

#include <wrl/client.h>

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/service_constants.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/wireguard_utils.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/brave_vpn_interactive_strings_en.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/brave_vpn_tray_command_ids.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/interactive_utils.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/resources/resource.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/status_icon_win.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/status_tray_win.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

namespace {

constexpr char kBraveAccountURL[] = "http://account.brave.com/";
constexpr char kAboutBraveVPNURL[] = "https://brave.com/firewall-vpn/";

}  // namespace

InteractiveMain* InteractiveMain::GetInstance() {
  static base::NoDestructor<InteractiveMain> instance;
  return instance.get();
}

InteractiveMain::InteractiveMain() = default;

InteractiveMain::~InteractiveMain() = default;

void InteractiveMain::SetupStatusIcon() {
  status_tray_ = std::make_unique<StatusTrayWin>();
  int status_icon_id = brave::ShouldUseDarkTheme() ? IDR_BRAVE_VPN_TRAY_LIGHT
                                                   : IDR_BRAVE_VPN_TRAY_DARK;
  status_icon_ = status_tray_->CreateStatusIcon(
      brave::GetIconFromResources(status_icon_id, {64, 64}),
      brave::kBraveVpnIconTooltip);

  status_icon_->SetContextMenu(std::make_unique<BraveVpnMenuModel>(this));
}

void InteractiveMain::ExecuteCommand(int command_id, int event_flags) {
  switch (command_id) {
    case IDC_BRAVE_VPN_TRAY_EXIT_ICON:
      SignalExit();
      break;
    case IDC_BRAVE_VPN_TRAY_CONNECT_VPN_ITEM:
      brave_vpn::wireguard::EnableBraveVpnWireguardService(
          "", base::BindOnce(&InteractiveMain::OnConnect,
                             weak_factory_.GetWeakPtr()));
      break;
    case IDC_BRAVE_VPN_TRAY_DISCONNECT_VPN_ITEM:
      brave_vpn::wireguard::DisableBraveVpnWireguardService(base::BindOnce(
          &InteractiveMain::OnDisconnect, weak_factory_.GetWeakPtr()));
      break;
    case IDC_BRAVE_VPN_TRAY_MANAGE_ACCOUNT_ITEM:
      brave::OpenURLInBrowser(kBraveAccountURL);
      break;
    case IDC_BRAVE_VPN_TRAY_ABOUT_ITEM:
      brave::OpenURLInBrowser(kAboutBraveVPNURL);
      break;
  }
}

void InteractiveMain::OnConnect(bool success) {
  VLOG(1) << __func__ << ":" << success;
}

void InteractiveMain::OnDisconnect(bool success) {
  VLOG(1) << __func__ << ":" << success;
}

HRESULT InteractiveMain::Run() {
  if (!wireguard::IsVPNTrayIconAllowed() ||
      !wireguard::GetLastUsedConfigPath().has_value() ||
      !StatusTrayWin::IconWindowExists()) {
    VLOG(1) << "No config available to connect.";
    return S_OK;
  }
  base::SingleThreadTaskExecutor task_executor(base::MessagePumpType::UI);
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams(
      "Braver VPN interactive process");

  base::RunLoop loop;
  quit_ = loop.QuitClosure();
  SetupStatusIcon();
  loop.Run();

  return S_OK;
}

void InteractiveMain::SignalExit() {
  std::move(quit_).Run();
}

}  // namespace brave_vpn
