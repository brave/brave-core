/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_win.h"

#include <Windows.h>

#include <vector>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_hstring.h"
#include "base/win/windows_version.h"
#include "bat/ads/configuration_info_log.h"
#include "chrome/common/chrome_features.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"

namespace brave_ads {

namespace {

using WindowParamsContainer = std::vector<ads::WindowParams>;

std::string GetWindowTextAsString(HWND hwnd) {
  const size_t num_chars = ::GetWindowTextLength(hwnd);
  if (!num_chars)
    return {};
  std::vector<wchar_t> tmp(num_chars + 1);
  if (!::GetWindowText(hwnd, &tmp.front(), tmp.size()))
    return {};

  return base::WideToUTF8(base::WStringPiece(&tmp.front(), num_chars));
}

BOOL CALLBACK FillContainer(HWND hwnd, LPARAM lparam) {
  if (!IsWindowVisible(hwnd)) {
    return TRUE;
  }
  WindowParamsContainer* params_container =
      reinterpret_cast<WindowParamsContainer*>(lparam);
  ads::WindowParams params;
  params.title = GetWindowTextAsString(hwnd);

  if (params.title.empty()) {
    return TRUE;
  }

  params_container->push_back(params);

  return TRUE;
}

BOOL CALLBACK TopWindowsEnumProc(HWND hwnd, LPARAM lparam) {
  FillContainer(hwnd, lparam);

  return TRUE;
}

void LogOsWindows() {
  WindowParamsContainer params_container;
  ::EnumWindows(TopWindowsEnumProc,
                reinterpret_cast<LPARAM>(&params_container));
  ads::WriteConfigurationInfoLog(params_container);
}

}  // namespace

// Copied from ntdef.h as not available in the Windows SDK and is required to
// detect if Focus Assist is enabled. Focus Assist is currently undocumented

typedef __success(return >= 0) LONG NTSTATUS;

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef struct _WNF_STATE_NAME {
  ULONG Data[2];
} WNF_STATE_NAME;

typedef struct _WNF_STATE_NAME* PWNF_STATE_NAME;
typedef const struct _WNF_STATE_NAME* PCWNF_STATE_NAME;

typedef struct _WNF_TYPE_ID {
  GUID TypeId;
} WNF_TYPE_ID, *PWNF_TYPE_ID;

typedef const WNF_TYPE_ID* PCWNF_TYPE_ID;

typedef ULONG WNF_CHANGE_STAMP, *PWNF_CHANGE_STAMP;

typedef NTSTATUS(NTAPI* PNTQUERYWNFSTATEDATA)(
    _In_ PWNF_STATE_NAME StateName,
    _In_opt_ PWNF_TYPE_ID TypeId,
    _In_opt_ const VOID* ExplicitScope,
    _Out_ PWNF_CHANGE_STAMP ChangeStamp,
    _Out_writes_bytes_to_opt_(*BufferSize, *BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize);

enum FocusAssistResult {
  NOT_SUPPORTED = -2,
  FAILED = -1,
  OFF = 0,
  PRIORITY_ONLY = 1,
  ALARMS_ONLY = 2
};

///////////////////////////////////////////////////////////////////////////////

NotificationHelperWin::NotificationHelperWin() {
  timer_.Start(FROM_HERE, base::TimeDelta::FromMinutes(1), this,
               &NotificationHelperWin::LogConfigurationInfo);
}

NotificationHelperWin::~NotificationHelperWin() = default;

void NotificationHelperWin::LogConfigurationInfo() {
  std::pair<bool, std::string> status = IsNotificationsEnabled();
  ads::NativeNotificationsStatus notifications_status;
  notifications_status.enabled = status.first;
  notifications_status.reason = status.second;
  ads::WriteConfigurationInfoLog(notifications_status);

  status = IsFocusAssistEnabled();
  ads::FocusAssistStatus focus_assist_status;
  focus_assist_status.enabled = status.first;
  focus_assist_status.reason = status.second;
  ads::WriteConfigurationInfoLog(focus_assist_status);

  LogOsWindows();
}

bool NotificationHelperWin::CanShowNativeNotifications() {
  if (!base::FeatureList::IsEnabled(::features::kNativeNotifications)) {
    LOG(WARNING) << "Native notifications feature is disabled";
    return false;
  }

  if (base::win::GetVersion() < base::win::Version::WIN10_RS4) {
    // There was a Microsoft bug in Windows 10 prior to version 1803, build
    // 17134 (i.e. VERSION_WIN10_RS4) causing endless loops in displaying
    // notifications. It significantly amplified the memory and CPU usage.
    // Therefore, Windows 10 native notifications in Chromium are only enabled
    // for version 1803, build 17134 and later
    LOG(WARNING) << "Native notifications are not supported prior to Windows "
                    "10 build 17134";
    return false;
  }

  if (!IsNotificationsEnabled().first) {
    return false;
  }

  if (IsFocusAssistEnabled().first) {
    return false;
  }

  return true;
}

bool NotificationHelperWin::CanShowBackgroundNotifications() const {
  return true;
}

bool NotificationHelperWin::ShowMyFirstAdNotification() {
  return false;
}

NotificationHelperWin* NotificationHelperWin::GetInstanceImpl() {
  return base::Singleton<NotificationHelperWin>::get();
}

NotificationHelper* NotificationHelper::GetInstanceImpl() {
  return NotificationHelperWin::GetInstanceImpl();
}

///////////////////////////////////////////////////////////////////////////////

std::pair<bool, std::string> NotificationHelperWin::IsFocusAssistEnabled()
    const {
  std::string reason;
  const auto nt_query_wnf_state_data_func =
      GetProcAddress(GetModuleHandle(L"ntdll"), "NtQueryWnfStateData");

  const auto nt_query_wnf_state_data =
      PNTQUERYWNFSTATEDATA(nt_query_wnf_state_data_func);

  if (!nt_query_wnf_state_data) {
    reason = "Failed to get pointer to NtQueryWnfStateData function";
    LOG(ERROR) << reason;
    return std::make_pair(false, reason);
  }

  // State name for Focus Assist
  WNF_STATE_NAME WNF_SHEL_QUIETHOURS_ACTIVE_PROFILE_CHANGED{0xA3BF1C75,
                                                            0xD83063E};

  WNF_CHANGE_STAMP change_stamp = {// Not used but is required
                                   0};

  DWORD buffer = 0;
  ULONG buffer_size = sizeof(buffer);

  if (!NT_SUCCESS(nt_query_wnf_state_data(
          &WNF_SHEL_QUIETHOURS_ACTIVE_PROFILE_CHANGED, nullptr, nullptr,
          &change_stamp, &buffer, &buffer_size))) {
    reason = "Failed to get status of Focus Assist";
    LOG(ERROR) << reason;
    return std::make_pair(false, reason);
  }

  auto result = (FocusAssistResult)buffer;

  switch (result) {
    case NOT_SUPPORTED: {
      reason = "Focus Assist is unsupported";
      LOG(WARNING) << reason;
      return std::make_pair(false, reason);
    }

    case FAILED: {
      reason = "Failed to determine Focus Assist status";
      LOG(WARNING) << reason;
      return std::make_pair(false, reason);
    }

    case OFF: {
      reason = "Focus Assist is disabled";
      LOG(INFO) << reason;
      return std::make_pair(false, reason);
    }

    case PRIORITY_ONLY: {
      reason = "Focus Assist is set to priority only";
      LOG(INFO) << reason;
      return std::make_pair(true, reason);
    }

    case ALARMS_ONLY: {
      reason = "Focus Assist is set to alarms only";
      LOG(INFO) << reason;
      return std::make_pair(true, reason);
    }
  }
}

std::pair<bool, std::string> NotificationHelperWin::IsNotificationsEnabled() {
  std::string reason;
  HRESULT hr = InitializeToastNotifier();
  auto* notifier = notifier_.Get();
  if (!notifier || FAILED(hr)) {
    reason = "Failed to initialize toast notifier";
    LOG(ERROR) << reason;
    return std::make_pair(true, reason);
  }

  ABI::Windows::UI::Notifications::NotificationSetting setting;
  hr = notifier->get_Setting(&setting);
  if (FAILED(hr)) {
    reason = "Failed to get notification settings from toast notifier";
    LOG(ERROR) << reason;
    return std::make_pair(true, reason);
  }

  switch (setting) {
    case ABI::Windows::UI::Notifications::NotificationSetting_Enabled: {
      reason = "Notifications are enabled";
      LOG(INFO) << reason;
      return std::make_pair(true, reason);
    }

    case ABI::Windows::UI::Notifications::NotificationSetting_DisabledForUser: {
      reason = "Notifications disabled for user";
      LOG(WARNING) << reason;
      return std::make_pair(false, reason);
    }

    case ABI::Windows::UI::Notifications::
        NotificationSetting_DisabledForApplication: {
      reason = "Notifications disabled for application";
      LOG(WARNING) << reason;
      return std::make_pair(false, reason);
    }

    case ABI::Windows::UI::Notifications::
        NotificationSetting_DisabledByGroupPolicy: {
      reason = "Notifications disabled by group policy";
      LOG(WARNING) << reason;
      return std::make_pair(false, reason);
    }

    case ABI::Windows::UI::Notifications::
        NotificationSetting_DisabledByManifest: {
      reason = "Notifications disabled by manifest";
      LOG(WARNING) << reason;
      return std::make_pair(false, reason);
    }
  }
}

std::wstring NotificationHelperWin::GetAppId() const {
  return ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall());
}

HRESULT NotificationHelperWin::InitializeToastNotifier() {
  Microsoft::WRL::ComPtr<
      ABI::Windows::UI::Notifications::IToastNotificationManagerStatics>
      manager;

  HRESULT hr = CreateActivationFactory(
      RuntimeClass_Windows_UI_Notifications_ToastNotificationManager,
      manager.GetAddressOf());

  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to create activation factory";
    return hr;
  }

  auto application_id = base::win::ScopedHString::Create(GetAppId());
  hr = manager->CreateToastNotifierWithId(application_id.get(), &notifier_);
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to create toast notifier";
    return hr;
  }

  return hr;
}

// Templated wrapper for ABI::Windows::Foundation::GetActivationFactory()
template <unsigned int size, typename T>
HRESULT NotificationHelperWin::CreateActivationFactory(
    wchar_t const (&class_name)[size],
    T** object) const {
  auto ref_class_name = base::win::ScopedHString::Create(
      base::WStringPiece(class_name, size - 1));

  return base::win::RoGetActivationFactory(ref_class_name.get(),
                                           IID_PPV_ARGS(object));
}

}  // namespace brave_ads
