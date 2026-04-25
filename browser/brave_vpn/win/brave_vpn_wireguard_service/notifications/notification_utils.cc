/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/notifications/notification_utils.h"

#include <objbase.h>
#include <windows.ui.notifications.h>
#include <wrl/client.h>
#include <wrl/event.h>

#include <string_view>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/types/expected.h"
#include "base/win/com_init_util.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_hstring.h"
#include "base/win/win_util.h"
#include "chrome/install_static/install_util.h"

namespace mswr = Microsoft::WRL;
namespace winui = ABI::Windows::UI;
namespace winxml = ABI::Windows::Data::Xml;

namespace brave_vpn {

namespace {
constexpr wchar_t kNotificationTemplate[] =
    L"<toast><visual><binding "
    L"template='ToastGeneric'><text>{text}</text></binding></visual></toast>";

// Templated wrapper for winfoundtn::GetActivationFactory().
template <unsigned int size>
HRESULT CreateActivationFactory(wchar_t const (&class_name)[size],
                                const IID& iid,
                                void** factory) {
  base::win::ScopedHString ref_class_name =
      base::win::ScopedHString::Create(std::wstring_view(class_name, size - 1));
  return base::win::RoGetActivationFactory(ref_class_name.get(), iid, factory);
}

mswr::ComPtr<winui::Notifications::IToastNotification> GetToastNotification(
    const std::wstring& xml_template) {
  base::win::ScopedHString ref_class_name = base::win::ScopedHString::Create(
      RuntimeClass_Windows_Data_Xml_Dom_XmlDocument);
  mswr::ComPtr<IInspectable> inspectable;
  HRESULT hr =
      base::win::RoActivateInstance(ref_class_name.get(), &inspectable);
  if (FAILED(hr)) {
    return nullptr;
  }

  mswr::ComPtr<winxml::Dom::IXmlDocumentIO> document_io;
  hr = inspectable.As(&document_io);
  if (FAILED(hr)) {
    return nullptr;
  }

  base::win::ScopedHString ref_template =
      base::win::ScopedHString::Create(xml_template);
  hr = document_io->LoadXml(ref_template.get());
  if (FAILED(hr)) {
    return nullptr;
  }

  mswr::ComPtr<winxml::Dom::IXmlDocument> document;
  hr = document_io.As<winxml::Dom::IXmlDocument>(&document);
  if (FAILED(hr)) {
    return nullptr;
  }

  mswr::ComPtr<winui::Notifications::IToastNotificationFactory>
      toast_notification_factory;
  hr = CreateActivationFactory(
      RuntimeClass_Windows_UI_Notifications_ToastNotification,
      IID_PPV_ARGS(&toast_notification_factory));
  if (FAILED(hr)) {
    return nullptr;
  }

  mswr::ComPtr<winui::Notifications::IToastNotification> toast_notification;
  hr = toast_notification_factory->CreateToastNotification(document.Get(),
                                                           &toast_notification);
  if (FAILED(hr)) {
    return nullptr;
  }
  return toast_notification;
}

base::expected<void, std::string> ShowDesktopNotificationImpl(
    const std::wstring& content) {
  base::win::AssertComInitialized();

  mswr::ComPtr<winui::Notifications::IToastNotificationManagerStatics>
      toast_manager;
  HRESULT hr = CreateActivationFactory(
      RuntimeClass_Windows_UI_Notifications_ToastNotificationManager,
      IID_PPV_ARGS(&toast_manager));
  if (FAILED(hr)) {
    return base::unexpected("Failed to create ToastNotificationManager");
  }

  mswr::ComPtr<winui::Notifications::IToastNotifier> notifier;
  base::win::ScopedHString application_id =
      base::win::ScopedHString::Create(install_static::GetBaseAppId());
  hr =
      toast_manager->CreateToastNotifierWithId(application_id.get(), &notifier);
  if (FAILED(hr)) {
    return base::unexpected("Failed to create IToastNotifier");
  }

  winui::Notifications::NotificationSetting setting;
  hr = notifier->get_Setting(&setting);
  if (setting != ABI::Windows::UI::Notifications::NotificationSetting_Enabled) {
    return base::unexpected("Notifications disabled for app");
  }

  mswr::ComPtr<winui::Notifications::IToastNotification> toast =
      GetToastNotification(content);
  if (!toast) {
    return base::unexpected("Failed to create IToastNotification");
  }
  hr = notifier->Show(toast.Get());
  if (FAILED(hr)) {
    return base::unexpected("Failed to show IToastNotification");
  }
  return base::ok();
}

}  // namespace

void ShowDesktopNotification(const std::wstring& text) {
  std::wstring content(kNotificationTemplate);
  base::ReplaceSubstringsAfterOffset(&content, 0, L"{text}", text);
  auto result = ShowDesktopNotificationImpl(content);
  if (!result.has_value()) {
    VLOG(1) << result.error();
  }
}

}  // namespace brave_vpn
