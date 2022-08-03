/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_WIN_H_
#define BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_WIN_H_

#include <windows.ui.notifications.h>
#include <wrl/event.h>

#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_ads/notification_helper/notification_helper_impl.h"

namespace brave_ads {

class NotificationHelperImplWin
    : public NotificationHelperImpl,
      public base::SupportsWeakPtr<NotificationHelperImplWin> {
 public:
  NotificationHelperImplWin(const NotificationHelperImplWin&) = delete;
  NotificationHelperImplWin& operator=(const NotificationHelperImplWin&) =
      delete;
  ~NotificationHelperImplWin() override;

 protected:
  friend class NotificationHelper;

  NotificationHelperImplWin();

 private:
  bool IsFocusAssistEnabled() const;

  bool IsNotificationsEnabled();

  std::wstring GetAppId() const;

  HRESULT InitializeToastNotifier();

  template <unsigned int size, typename T>
  HRESULT CreateActivationFactory(wchar_t const (&class_name)[size],
                                  T** object) const;

  Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotifier>
      notifier_;

  // NotificationHelperImpl:
  bool CanShowNotifications() override;
  bool CanShowSystemNotificationsWhileBrowserIsBackgrounded() const override;

  bool ShowOnboardingNotification() override;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_WIN_H_
