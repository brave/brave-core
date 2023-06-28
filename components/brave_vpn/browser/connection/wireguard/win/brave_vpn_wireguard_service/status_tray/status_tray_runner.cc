/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/status_tray_runner.h"

#include <windows.h>  // Should be before shellapi.h
#include <wrl/client.h>

#include <shellapi.h>

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/wireguard_utils.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/brave_vpn_tray_command_ids.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/brave_vpn_tray_strings_en.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/resources/resource.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/status_icon/icon_utils.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/status_icon/status_icon.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/status_icon/status_tray.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/native_theme/native_theme.h"

namespace brave_vpn {

namespace {

void OpenURLInBrowser(const char* url) {
  if (reinterpret_cast<ULONG_PTR>(
          ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL)) <= 32) {
    VLOG(1) << "Failed to open url in browser:" << url;
    return;
  }
}

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
  bool dark_theme =
      ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors();
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

StatusTrayRunner* StatusTrayRunner::GetInstance() {
  static base::NoDestructor<StatusTrayRunner> instance;
  return instance.get();
}

StatusTrayRunner::StatusTrayRunner() = default;

StatusTrayRunner::~StatusTrayRunner() = default;

void StatusTrayRunner::SetupStatusIcon() {
  status_tray_ = std::make_unique<StatusTray>();
  auto connected = IsTunnelServiceRunning();
  status_tray_->CreateStatusIcon(GetStatusTrayIcon(connected, false),
                                 GetStatusIconTooltip(connected, false));
  auto* status_icon = status_tray_->GetStatusIcon();
  if (status_icon) {
    status_icon->SetContextMenu(std::make_unique<TrayMenuModel>(this));
  }
}

void StatusTrayRunner::ExecuteCommand(int command_id, int event_flags) {
  switch (command_id) {
    case IDC_BRAVE_VPN_TRAY_EXIT_ICON:
      SignalExit();
      break;
    case IDC_BRAVE_VPN_TRAY_CONNECT_VPN_ITEM:
      wireguard::EnableBraveVpnWireguardService(
          "", base::BindOnce(&StatusTrayRunner::OnConnected,
                             weak_factory_.GetWeakPtr()));
      break;
    case IDC_BRAVE_VPN_TRAY_DISCONNECT_VPN_ITEM:
      wireguard::DisableBraveVpnWireguardService(base::BindOnce(
          &StatusTrayRunner::OnDisconnected, weak_factory_.GetWeakPtr()));
      break;
    case IDC_BRAVE_VPN_TRAY_MANAGE_ACCOUNT_ITEM:
      OpenURLInBrowser(kManageUrlProd);
      break;
    case IDC_BRAVE_VPN_TRAY_ABOUT_ITEM:
      OpenURLInBrowser(kAboutUrl);
      break;
  }
}

bool StatusTrayRunner::IsTunnelServiceRunning() const {
  if (service_running_for_testing_.has_value()) {
    return service_running_for_testing_.value();
  }
  return wireguard::IsBraveVPNWireguardTunnelServiceRunning();
}

void StatusTrayRunner::OnMenuWillShow(ui::SimpleMenuModel* source) {
  auto connected = IsTunnelServiceRunning();
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

void StatusTrayRunner::OnConnected(bool success) {
  VLOG(1) << __func__ << ":" << success;
  UpdateIconState(!success);
}

void StatusTrayRunner::UpdateIconState(bool error) {
  if (!status_tray_ || !status_tray_->GetStatusIcon()) {
    return;
  }
  auto connected = IsTunnelServiceRunning();
  status_tray_->GetStatusIcon()->UpdateState(
      GetStatusTrayIcon(connected, error),
      GetStatusIconTooltip(connected, error));
}

void StatusTrayRunner::OnDisconnected(bool success) {
  VLOG(1) << __func__ << ":" << success;
  UpdateIconState(!success);
}

HRESULT StatusTrayRunner::Run() {
  if (!wireguard::GetLastUsedConfigPath().has_value() ||
      StatusTray::IconWindowExists()) {
    return S_OK;
  }

  base::SingleThreadTaskExecutor task_executor(base::MessagePumpType::UI);
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams(
      "Brave VPN Wireguard status tray process");
  SetupStatusIcon();
  base::RunLoop loop;
  quit_ = loop.QuitClosure();
  loop.Run();
  return S_OK;
}

void StatusTrayRunner::SignalExit() {
  status_tray_.reset();
  std::move(quit_).Run();
}

}  // namespace brave_vpn
