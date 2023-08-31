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

int GetStatusIconTooltip(brave_vpn::mojom::ConnectionState state) {
  switch (state) {
    case brave_vpn::mojom::ConnectionState::CONNECTING:
      return IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_CONNECTING;
    case brave_vpn::mojom::ConnectionState::CONNECTED:
      return IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_CONNECTED;
    case brave_vpn::mojom::ConnectionState::DISCONNECTING:
      return IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_DISCONNECTING;
    case brave_vpn::mojom::ConnectionState::DISCONNECTED:
      return IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_DISCONNECTED;
    case brave_vpn::mojom::ConnectionState::CONNECT_FAILED:
    case brave_vpn::mojom::ConnectionState::CONNECT_NOT_ALLOWED:
      return IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_ERROR;
  }

  NOTREACHED_NORETURN();
}

int GetStatusTrayIcon(brave_vpn::mojom::ConnectionState state) {
  bool dark_theme =
      ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors();
  switch (state) {
    case brave_vpn::mojom::ConnectionState::CONNECTED:
      return dark_theme ? IDR_BRAVE_VPN_TRAY_LIGHT_CONNECTED
                        : IDR_BRAVE_VPN_TRAY_DARK_CONNECTED;
    case brave_vpn::mojom::ConnectionState::DISCONNECTING:
    case brave_vpn::mojom::ConnectionState::DISCONNECTED:
      return dark_theme ? IDR_BRAVE_VPN_TRAY_LIGHT : IDR_BRAVE_VPN_TRAY_DARK;
    case brave_vpn::mojom::ConnectionState::CONNECTING:
      return dark_theme ? IDR_BRAVE_VPN_TRAY_LIGHT_CONNECTING
                        : IDR_BRAVE_VPN_TRAY_DARK_CONNECTING;
    case brave_vpn::mojom::ConnectionState::CONNECT_FAILED:
    case brave_vpn::mojom::ConnectionState::CONNECT_NOT_ALLOWED:
      return dark_theme ? IDR_BRAVE_VPN_TRAY_LIGHT_ERROR
                        : IDR_BRAVE_VPN_TRAY_DARK_ERROR;
  }

  NOTREACHED_NORETURN();
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
  current_state_ = IsTunnelServiceRunning()
                       ? brave_vpn::mojom::ConnectionState::CONNECTED
                       : brave_vpn::mojom::ConnectionState::DISCONNECTED;
  status_tray_->CreateStatusIcon(
      GetIconFromResources(GetStatusTrayIcon(current_state_.value()), {64, 64}),
      l10n_util::GetStringUTF16(GetStatusIconTooltip(current_state_.value())));
  auto* status_icon = status_tray_->GetStatusIcon();
  if (status_icon) {
    status_icon->SetContextMenu(std::make_unique<TrayMenuModel>(this));
  }
  SubscribeForWireguardNotifications(
      IsTunnelServiceRunning() ? GetBraveVpnWireguardTunnelServiceName()
                               : GetBraveVpnWireguardServiceName());
  UpdateConnectionState();
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

brave_vpn::mojom::ConnectionState StatusTrayRunner::GetConnectionState() {
  if (IsTunnelServiceRunning()) {
    return brave_vpn::mojom::ConnectionState::CONNECTED;
  }

  auto state_from_storage = brave_vpn::wireguard::GetConnectionState();
  if (state_from_storage.has_value()) {
    return static_cast<brave_vpn::mojom::ConnectionState>(
        state_from_storage.value());
  }

  return brave_vpn::mojom::ConnectionState::DISCONNECTED;
}

bool StatusTrayRunner::IsIconCreated() {
  return status_tray_ && status_tray_->GetStatusIcon();
}

void StatusTrayRunner::UpdateConnectionState() {
  auto state = GetConnectionState();
  if (state == brave_vpn::mojom::ConnectionState::CONNECTED) {
    // Check if we have obsolete connected state in storage.
    state = IsTunnelServiceRunning()
                ? state
                : brave_vpn::mojom::ConnectionState::DISCONNECTED;
    // if Tunnel service launched it means we have connected state and should
    // reset storage states because it could be expired from closed browser.
    wireguard::WriteConnectionState(static_cast<int>(state));
  }

  if (current_state_ == state) {
    return;
  }
  current_state_ = state;
  SetIconState(GetStatusTrayIcon(state), GetStatusIconTooltip(state));
}

void StatusTrayRunner::SetIconState(int icon_id, int tooltip_id) {
  if (callback_for_testing_) {
    callback_for_testing_.Run(icon_id, tooltip_id);
    return;
  }

  if (!IsIconCreated()) {
    return;
  }
  status_tray_->GetStatusIcon()->UpdateState(
      GetIconFromResources(icon_id, {64, 64}),
      l10n_util::GetStringUTF16(tooltip_id));
}

void StatusTrayRunner::OnWireguardServiceStateChanged(int mask) {
  UpdateConnectionState();
  SubscribeForWireguardNotifications(
      IsTunnelServiceRunning() ? GetBraveVpnWireguardTunnelServiceName()
                               : GetBraveVpnWireguardServiceName());
}

void StatusTrayRunner::OnDisconnected(bool success) {
  VLOG(1) << __func__ << ":" << success;
}

void StatusTrayRunner::OnStorageUpdated() {
  // Checking if tray icon enabled from Brave-> App menu -> Brave VPN -> Show
  // VPN tray icon.
  if (!wireguard::IsVPNTrayIconEnabled()) {
    SignalExit();
  }

  // Checks if wireguard protocol activated in brave://settings/system.
  if (!wireguard::IsWireguardActive()) {
    SignalExit();
  }

  UpdateConnectionState();
  storage_.StartWatching(base::BindRepeating(
      &StatusTrayRunner::OnStorageUpdated, weak_factory_.GetWeakPtr()));
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
