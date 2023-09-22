/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper_impl_win.h"

#include <Windows.h>

#include <string_view>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_hstring.h"
#include "base/win/windows_version.h"
#include "chrome/common/chrome_features.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"

namespace brave_ads {

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

NotificationHelperImplWin::NotificationHelperImplWin() = default;

NotificationHelperImplWin::~NotificationHelperImplWin() = default;

bool NotificationHelperImplWin::CanShowNotifications() {
  if (!base::FeatureList::IsEnabled(::features::kNativeNotifications)) {
    VLOG(1) << "Native notifications feature is disabled";
    return false;
  }

  if (base::win::GetVersion() < base::win::Version::WIN10_RS4) {
    // There was a Microsoft bug in Windows 10 prior to version 1803, build
    // 17134 (i.e. VERSION_WIN10_RS4) causing endless loops in displaying
    // notifications. It significantly amplified the memory and CPU usage.
    // Therefore, Windows 10 native notifications in Chromium are only enabled
    // for version 1803, build 17134 and later
    VLOG(1) << "Native notifications are not supported prior to Windows 10 "
               "build 17134";
    return false;
  }

  if (!IsNotificationsEnabled()) {
    return false;
  }

  if (IsFocusAssistEnabled()) {
    return false;
  }

  return true;
}

bool NotificationHelperImplWin::
    CanShowSystemNotificationsWhileBrowserIsBackgrounded() const {
  return true;
}

bool NotificationHelperImplWin::ShowOnboardingNotification() {
  return false;
}

///////////////////////////////////////////////////////////////////////////////

bool NotificationHelperImplWin::IsFocusAssistEnabled() const {
  const auto nt_query_wnf_state_data_func =
      GetProcAddress(GetModuleHandle(L"ntdll"), "NtQueryWnfStateData");

  const auto nt_query_wnf_state_data =
      PNTQUERYWNFSTATEDATA(nt_query_wnf_state_data_func);

  if (!nt_query_wnf_state_data) {
    VLOG(0) << "Failed to get pointer to NtQueryWnfStateData function";
    return false;
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
    VLOG(0) << "Failed to get status of Focus Assist";
    return false;
  }

  auto result = (FocusAssistResult)buffer;

  switch (result) {
    case NOT_SUPPORTED: {
      VLOG(1) << "Focus Assist is unsupported";
      return false;
    }

    case FAILED: {
      VLOG(1) << "Failed to determine Focus Assist status";
      return false;
    }

    case OFF: {
      VLOG(1) << "Focus Assist is disabled";
      return false;
    }

    case PRIORITY_ONLY: {
      VLOG(1) << "Focus Assist is set to priority only";
      return true;
    }

    case ALARMS_ONLY: {
      VLOG(1) << "Focus Assist is set to alarms only";
      return true;
    }
  }
}

bool NotificationHelperImplWin::IsNotificationsEnabled() {
  HRESULT hr = InitializeToastNotifier();
  auto* toast_notifier = toast_notifier_.Get();
  if (!toast_notifier || FAILED(hr)) {
    VLOG(0) << "Failed to initialize toast notifier";
    return true;
  }

  ABI::Windows::UI::Notifications::NotificationSetting notification_setting;
  hr = toast_notifier->get_Setting(&notification_setting);
  if (FAILED(hr)) {
    VLOG(0) << "Failed to get notification settings from toast notifier";
    return true;
  }

  switch (notification_setting) {
    case ABI::Windows::UI::Notifications::NotificationSetting_Enabled: {
      VLOG(1) << "Notifications are enabled";
      return true;
    }

    case ABI::Windows::UI::Notifications::NotificationSetting_DisabledForUser: {
      VLOG(1) << "Notifications disabled for user";
      return false;
    }

    case ABI::Windows::UI::Notifications::
        NotificationSetting_DisabledForApplication: {
      VLOG(1) << "Notifications disabled for application";
      return false;
    }

    case ABI::Windows::UI::Notifications::
        NotificationSetting_DisabledByGroupPolicy: {
      VLOG(1) << "Notifications disabled by group policy";
      return false;
    }

    case ABI::Windows::UI::Notifications::
        NotificationSetting_DisabledByManifest: {
      VLOG(1) << "Notifications disabled by manifest";
      return false;
    }
  }
}

std::wstring NotificationHelperImplWin::GetAppId() const {
  return ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall());
}

HRESULT NotificationHelperImplWin::InitializeToastNotifier() {
  Microsoft::WRL::ComPtr<
      ABI::Windows::UI::Notifications::IToastNotificationManagerStatics>
      toast_notification_manager;

  HRESULT hr = CreateActivationFactory(
      RuntimeClass_Windows_UI_Notifications_ToastNotificationManager,
      IID_PPV_ARGS(&toast_notification_manager));

  if (FAILED(hr)) {
    VLOG(0) << "Failed to create activation factory";
    return hr;
  }

  auto application_id = base::win::ScopedHString::Create(GetAppId());
  hr = toast_notification_manager->CreateToastNotifierWithId(
      application_id.get(), &toast_notifier_);
  if (FAILED(hr)) {
    VLOG(0) << "Failed to create toast notifier";
    return hr;
  }

  return hr;
}

// Templated wrapper for ABI::Windows::Foundation::GetActivationFactory()
template <unsigned int size>
HRESULT NotificationHelperImplWin::CreateActivationFactory(
    wchar_t const (&class_name)[size],
    const IID& iid,
    void** factory) const {
  auto ref_class_name =
      base::win::ScopedHString::Create(std::wstring_view(class_name, size - 1));

  return base::win::RoGetActivationFactory(ref_class_name.get(), iid, factory);
}

}  // namespace brave_ads
