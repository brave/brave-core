/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <Windows.h>

#include "brave/components/brave_ads/browser/notification_helper_win.h"

#include "chrome/browser/fullscreen.h"
#include "base/win/windows_version.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_hstring.h"
#include "base/feature_list.h"
#include "chrome/common/chrome_features.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "base/logging.h"

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
} WNF_TYPE_ID, * PWNF_TYPE_ID;

typedef const WNF_TYPE_ID* PCWNF_TYPE_ID;

typedef ULONG WNF_CHANGE_STAMP, * PWNF_CHANGE_STAMP;

typedef NTSTATUS (NTAPI* PNTQUERYWNFSTATEDATA)(
  _In_ PWNF_STATE_NAME StateName,
  _In_opt_ PWNF_TYPE_ID TypeId,
  _In_opt_ const VOID* ExplicitScope,
  _Out_ PWNF_CHANGE_STAMP ChangeStamp,
  _Out_writes_bytes_to_opt_(* BufferSize, * BufferSize) PVOID Buffer,
  _Inout_ PULONG BufferSize);

enum FocusAssistResult {
  NOT_SUPPORTED = -2,
  FAILED = -1,
  OFF = 0,
  PRIORITY_ONLY = 1,
  ALARMS_ONLY = 2
};

///////////////////////////////////////////////////////////////////////////////

NotificationHelperWin::NotificationHelperWin() = default;

NotificationHelperWin::~NotificationHelperWin() = default;

bool NotificationHelperWin::ShouldShowNotifications() {
  if (IsFullScreenMode()) {
    LOG(WARNING) << "Notification not made: Full screen mode";
    return false;
  }

  if (base::win::GetVersion() < base::win::Version::WIN10_RS4) {
    // There was a Microsoft bug in Windows 10 prior to build 17134 (i.e.
    // VERSION_WIN10_RS4) causing endless loops in displaying notifications. It
    // significantly amplified the memory and CPU usage. Therefore, Windows 10
    // native notifications in Chromium are only enabled for build 17134 and
    // later
    LOG(WARNING) << "Native notifications are not supported on Windows prior"
        " to Windows 10 build 17134 so falling back to Message Center";
    return true;
  }

  if (!base::FeatureList::IsEnabled(features::kNativeNotifications)) {
    LOG(WARNING) << "Native notification feature is disabled so falling back to"
        " Message Center";
    return true;
  }

  if (IsFocusAssistEnabled()) {
    LOG(INFO) << "Notification not made: Focus assist is enabled";
    return false;
  }

  if (!IsNotificationsEnabled()) {
    LOG(INFO) << "Notification not made: Notifications are disabled";
    return false;
  }

  return true;
}

bool NotificationHelperWin::ShowMyFirstAdNotification() {
  return false;
}

bool NotificationHelperWin::CanShowBackgroundNotifications() const {
  return true;
}

NotificationHelperWin* NotificationHelperWin::GetInstanceImpl() {
  return base::Singleton<NotificationHelperWin>::get();
}

NotificationHelper* NotificationHelper::GetInstanceImpl() {
  return NotificationHelperWin::GetInstanceImpl();
}

///////////////////////////////////////////////////////////////////////////////

bool NotificationHelperWin::IsFocusAssistEnabled() const {
  const auto nt_query_wnf_state_data_func =
      GetProcAddress(GetModuleHandle(L"ntdll"), "NtQueryWnfStateData");

  const auto nt_query_wnf_state_data =
      PNTQUERYWNFSTATEDATA(nt_query_wnf_state_data_func);

  if (!nt_query_wnf_state_data) {
    LOG(ERROR) << "Failed to get pointer to NtQueryWnfStateData function";
    return false;
  }

  // State name for Focus Assist
  WNF_STATE_NAME WNF_SHEL_QUIETHOURS_ACTIVE_PROFILE_CHANGED {
    0xA3BF1C75, 0xD83063E
  };

  WNF_CHANGE_STAMP change_stamp = {  // Not used but is required
    0
  };

  DWORD buffer = 0;
  ULONG buffer_size = sizeof(buffer);

  if (!NT_SUCCESS(nt_query_wnf_state_data(
      &WNF_SHEL_QUIETHOURS_ACTIVE_PROFILE_CHANGED, nullptr, nullptr,
          &change_stamp, &buffer, &buffer_size))) {
    LOG(ERROR) << "Failed to get status of Focus Assist";
    return false;
  }

  auto result = (FocusAssistResult)buffer;

  switch (result) {
    case NOT_SUPPORTED: {
      LOG(WARNING) << "Focus Assist is unsupported";
      return false;
    }

    case FAILED: {
      LOG(WARNING) << "Failed to determine Focus Assist status";
      return false;
    }

    case OFF: {
      LOG(INFO) << "Focus Assist is disabled";
      return false;
    }

    case PRIORITY_ONLY: {
      LOG(INFO) << "Focus Assist is set to priority only";
      return true;
    }

    case ALARMS_ONLY: {
      LOG(INFO) << "Focus Assist is set to alarms only";
      return true;
    }
  }

  LOG(WARNING) << "Unknown Focus Assist status: " << result;
  return false;
}

bool NotificationHelperWin::IsNotificationsEnabled() {
  HRESULT hr = InitializeToastNotifier();
  auto* notifier = notifier_.Get();
  if (!notifier || FAILED(hr)) {
    LOG(ERROR) << "Failed to initialize toast notifier";
    return true;
  }

  ABI::Windows::UI::Notifications::NotificationSetting setting;
  hr = notifier->get_Setting(&setting);
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to get notification settings from toast notifier";
    return true;
  }

  switch (setting) {
    case ABI::Windows::UI::Notifications::
        NotificationSetting_Enabled: {
      LOG(INFO) << "Notifications are enabled";
      return true;
    }

    case ABI::Windows::UI::Notifications::
        NotificationSetting_DisabledForUser: {
      LOG(WARNING) << "Notifications disabled for user";
      return false;
    }

    case ABI::Windows::UI::Notifications::
        NotificationSetting_DisabledForApplication: {
      LOG(WARNING) << "Notifications disabled for application";
      return false;
    }

    case ABI::Windows::UI::Notifications::
        NotificationSetting_DisabledByGroupPolicy: {
      LOG(WARNING) << "Notifications disabled by group policy";
      return false;
    }

    case ABI::Windows::UI::Notifications::
        NotificationSetting_DisabledByManifest: {
      LOG(WARNING) << "Notifications disabled by manifest";
      return false;
    }
  }
}

base::string16 NotificationHelperWin::GetAppId() const {
  return ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall());
}

HRESULT NotificationHelperWin::InitializeToastNotifier() {
  Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::
      IToastNotificationManagerStatics> manager;

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
      base::StringPiece16(class_name, size - 1));

  return base::win::RoGetActivationFactory(
      ref_class_name.get(), IID_PPV_ARGS(object));
}

}  // namespace brave_ads
