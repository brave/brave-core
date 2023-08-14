/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_tray_runner.h"

#include <windows.h>  // Should be before shellapi.h
#include <wrl/client.h>

#include <shellapi.h>

#include <memory>
#include <string>
#include <utility>

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/brave_vpn_tray_command_ids.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/resources/resource.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/icon_utils.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/status_icon.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/status_tray.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"
#include "brave/components/brave_vpn/common/wireguard/win/storage_utils.h"
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils.h"
#include "components/grit/brave_components_strings.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
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
  return l10n_util::GetStringUTF16(
      active ? IDS_BRAVE_VPN_WIREGUARD_TRAY_STATUS_ITEM_ACTIVE
             : IDS_BRAVE_VPN_WIREGUARD_TRAY_STATUS_ITEM_INACTIVE);
}

std::u16string GetStatusIconTooltip(bool connected, bool error) {
  if (error) {
    return l10n_util::GetStringUTF16(
        IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_ERROR);
  }
  return l10n_util::GetStringUTF16(
      connected ? IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_CONNECTED
                : IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_DISCONNECTED);
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
  SubscribeForServiceStopNotifications(
      connected ? GetBraveVpnWireguardTunnelServiceName()
                : GetBraveVpnWireguardServiceName());
}

void StatusTrayRunner::ExecuteCommand(int command_id, int event_flags) {
  switch (command_id) {
    case IDC_BRAVE_VPN_TRAY_EXIT_ICON:
      wireguard::EnableVPNTrayIcon(false);
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
                    l10n_util::GetStringUTF16(
                        IDS_BRAVE_VPN_WIREGUARD_TRAY_DISCONNECT_ITEM));
  } else {
    source->AddItem(
        IDC_BRAVE_VPN_TRAY_CONNECT_VPN_ITEM,
        l10n_util::GetStringUTF16(IDS_BRAVE_VPN_WIREGUARD_TRAY_CONNECT_ITEM));
  }
  source->AddSeparator(ui::NORMAL_SEPARATOR);
  source->AddItem(IDC_BRAVE_VPN_TRAY_MANAGE_ACCOUNT_ITEM,
                  l10n_util::GetStringUTF16(
                      IDS_BRAVE_VPN_WIREGUARD_TRAY_MANAGE_ACCOUNT_ITEM));
  source->AddItem(
      IDC_BRAVE_VPN_TRAY_ABOUT_ITEM,
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_WIREGUARD_TRAY_ABOUT_ITEM));
  source->AddSeparator(ui::NORMAL_SEPARATOR);
  source->AddItem(
      IDC_BRAVE_VPN_TRAY_EXIT_ICON,
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_WIREGUARD_TRAY_REMOVE_ICON_ITEM));
}

void StatusTrayRunner::OnConnected(bool success) {
  VLOG(1) << __func__ << ":" << success;
}

void StatusTrayRunner::UpdateIconState(bool connected, bool error) {
  if (!status_tray_ || !status_tray_->GetStatusIcon()) {
    return;
  }

  status_tray_->GetStatusIcon()->UpdateState(
      GetStatusTrayIcon(connected, error),
      GetStatusIconTooltip(connected, error));
}

void StatusTrayRunner::OnServiceStateChanged(int mask) {
  auto connected = IsTunnelServiceRunning();
  UpdateIconState(connected, false);
  SubscribeForServiceStopNotifications(
      connected ? GetBraveVpnWireguardTunnelServiceName()
                : GetBraveVpnWireguardServiceName());
}

void StatusTrayRunner::SubscribeForServiceStopNotifications(
    const std::wstring& name) {
  if (service_watcher_) {
    if (service_watcher_->GetServiceName() == name) {
      service_watcher_->StartWatching();
      return;
    }
  }
  service_watcher_.reset(new brave::ServiceWatcher());
  if (!service_watcher_->Subscribe(
          name, SERVICE_NOTIFY_STOPPED,
          base::BindRepeating(&StatusTrayRunner::OnServiceStateChanged,
                              weak_factory_.GetWeakPtr()))) {
    VLOG(1) << "Unable to set service watcher for:" << name;
  }
}

void StatusTrayRunner::OnDisconnected(bool success) {
  VLOG(1) << __func__ << ":" << success;
}

void StatusTrayRunner::OnStorageUpdated() {
  VLOG(1) << __func__;
  if (!wireguard::IsVPNTrayIconEnabled()) {
    SignalExit();
  }
  if (!wireguard::IsWireguardActive()) {
    SignalExit();
  }
}

void StatusTrayRunner::SubscribeForStorageUpdates() {
  if (storage_.Create(
          HKEY_CURRENT_USER,
          wireguard::GetBraveVpnWireguardServiceRegistryStoragePath().c_str(),
          KEY_QUERY_VALUE | KEY_NOTIFY) != ERROR_SUCCESS) {
    return;
  }
  storage_.StartWatching(base::BindRepeating(
      &StatusTrayRunner::OnStorageUpdated, weak_factory_.GetWeakPtr()));
}

HRESULT StatusTrayRunner::Run() {
  if (!wireguard::GetLastUsedConfigPath().has_value()) {
    VLOG(1) << "Last used config not found.";
    return S_OK;
  }
  if (!wireguard::IsVPNTrayIconEnabled()) {
    VLOG(1) << "Tray icon was hidden by user.";
    return S_OK;
  }
  if (!wireguard::IsWireguardActive()) {
    VLOG(1) << "Wireguard VPN is not enabled in settings.";
    return S_OK;
  }

  if (StatusTray::IconWindowExists()) {
    VLOG(1) << "Tray icon is already visible.";
    return S_OK;
  }

  base::SingleThreadTaskExecutor task_executor(base::MessagePumpType::UI);
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams(
      "Brave VPN Wireguard status tray process");

  SetupStatusIcon();
  SubscribeForStorageUpdates();

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
