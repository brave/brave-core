/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/interactive_main.h"

#include <wrl/client.h>

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/service_constants.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/wireguard_utils.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/brave_vpn_tray_strings_en.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/brave_vpn_tray_command_ids.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/interactive_utils.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/resources/resource.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/status_icon/status_icon.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/status_icon/status_tray.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

namespace {

constexpr char kBraveAccountURL[] = "http://account.brave.com/";
constexpr char kAboutBraveVPNURL[] = "https://brave.com/firewall-vpn/";

std::u16string GetVpnStatusLabel(bool active) {
  std::u16string label = brave::kBraveVpnStatusItemName;
  label += (active ? brave::kBraveVpnActiveText : brave::kBraveVpnInactiveText);
  return label;
}

std::u16string GetStatusIconTooltip(bool connected, bool error) {
  if (error) {
    return brave::kBraveVpnIconTooltipError;
  }
  return connected ? brave::kBraveVpnIconTooltipConnected
                   : brave::kBraveVpnIconTooltip;
}

gfx::ImageSkia GetStatusTrayIcon(bool connected, bool error) {
  bool dark_theme = UseDarkTheme();
  if (error) {
    int status_icon_id = dark_theme ? IDR_BRAVE_VPN_TRAY_LIGHT_ERROR
                                    : IDR_BRAVE_VPN_TRAY_DARK_ERROR;
    return GetIconFromResources(status_icon_id, {64, 64});
  }
  int light_icon_id =
      connected ? IDR_BRAVE_VPN_TRAY_LIGHT_CONNECTED : IDR_BRAVE_VPN_TRAY_LIGHT;
  int dark_icon_id =
      connected ? IDR_BRAVE_VPN_TRAY_DARK_CONNECTED : IDR_BRAVE_VPN_TRAY_DARK;
  return GetIconFromResources(dark_theme ? light_icon_id : dark_icon_id,
                              {64, 64});
}

}  // namespace

InteractiveMain* InteractiveMain::GetInstance() {
  static base::NoDestructor<InteractiveMain> instance;
  return instance.get();
}

InteractiveMain::InteractiveMain() = default;

InteractiveMain::~InteractiveMain() = default;

void InteractiveMain::SetupStatusIcon() {
  status_tray_ = std::make_unique<StatusTrayWin>();
  auto connected = wireguard::IsBraveVPNWireguardTunnelServiceRunning();
  status_tray_->CreateStatusIcon(GetStatusTrayIcon(connected, false),
                                 GetStatusIconTooltip(connected, false));
  status_tray_->GetStatusIcon()->SetContextMenu(
      std::make_unique<BraveVpnMenuModel>(this));
}

void InteractiveMain::ExecuteCommand(int command_id, int event_flags) {
  switch (command_id) {
    case IDC_BRAVE_VPN_TRAY_EXIT_ICON:
      SignalExit();
      break;
    case IDC_BRAVE_VPN_TRAY_CONNECT_VPN_ITEM:
      wireguard::EnableBraveVpnWireguardService(
          "", base::BindOnce(&InteractiveMain::OnConnected,
                             weak_factory_.GetWeakPtr()));
      break;
    case IDC_BRAVE_VPN_TRAY_DISCONNECT_VPN_ITEM:
      wireguard::DisableBraveVpnWireguardService(base::BindOnce(
          &InteractiveMain::OnDisconnected, weak_factory_.GetWeakPtr()));
      break;
    case IDC_BRAVE_VPN_TRAY_MANAGE_ACCOUNT_ITEM:
      OpenURLInBrowser(kBraveAccountURL);
      break;
    case IDC_BRAVE_VPN_TRAY_ABOUT_ITEM:
      OpenURLInBrowser(kAboutBraveVPNURL);
      break;
  }
}

void InteractiveMain::OnMenuWillShow(ui::SimpleMenuModel* source) {
  auto connected = wireguard::IsBraveVPNWireguardTunnelServiceRunning();
  source->Clear();
  source->AddItem(IDC_BRAVE_VPN_TRAY_STATUS_ITEM, GetVpnStatusLabel(connected));
  source->SetEnabledAt(0, false);
  if (connected) {
    source->AddItem(IDC_BRAVE_VPN_TRAY_DISCONNECT_VPN_ITEM,
                    brave::kBraveVpnDisconnectItemName);
  } else {
    source->AddItem(IDC_BRAVE_VPN_TRAY_CONNECT_VPN_ITEM,
                    brave::kBraveVpnConnectItemName);
  }
  source->AddSeparator(ui::NORMAL_SEPARATOR);
  source->AddItem(IDC_BRAVE_VPN_TRAY_MANAGE_ACCOUNT_ITEM,
                  brave::kBraveVpnManageAccountItemName);
  source->AddItem(IDC_BRAVE_VPN_TRAY_ABOUT_ITEM, brave::kBraveVpnAboutItemName);
  source->AddSeparator(ui::NORMAL_SEPARATOR);
  source->AddItem(IDC_BRAVE_VPN_TRAY_EXIT_ICON, brave::kBraveVpnRemoveItemName);
}

void InteractiveMain::OnConnected(bool success) {
  VLOG(1) << __func__ << ":" << success;
  UpdateIconState(!success);
}

void InteractiveMain::UpdateIconState(bool error) {
  if (!status_tray_ || !status_tray_->GetStatusIcon()) {
    return;
  }
  auto connected = wireguard::IsBraveVPNWireguardTunnelServiceRunning();
  status_tray_->GetStatusIcon()->UpdateState(
      GetStatusTrayIcon(connected, error),
      GetStatusIconTooltip(connected, error));
}

void InteractiveMain::OnDisconnected(bool success) {
  VLOG(1) << __func__ << ":" << success;
  UpdateIconState(!success);
}

HRESULT InteractiveMain::Run() {
  if (!wireguard::GetLastUsedConfigPath().has_value() ||
      StatusTrayWin::IconWindowExists()) {
    return S_OK;
  }

  base::SingleThreadTaskExecutor task_executor(base::MessagePumpType::UI);
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams(
      "Brave VPN Wireguard interactive process");
  base::RunLoop loop;
  quit_ = loop.QuitClosure();
  SetupStatusIcon();
  loop.Run();
  return S_OK;
}

void InteractiveMain::SignalExit() {
  status_tray_.reset();
  std::move(quit_).Run();
}

}  // namespace brave_vpn
