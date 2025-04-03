/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_WIN_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_WIN_H_

#include <windows.ui.notifications.h>
#include <wrl/event.h>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper_impl.h"

namespace brave_ads {

class NotificationHelperImplWin final : public NotificationHelperImpl {
 public:
  NotificationHelperImplWin(const NotificationHelperImplWin&) = delete;
  NotificationHelperImplWin& operator=(const NotificationHelperImplWin&) =
      delete;

  ~NotificationHelperImplWin() override;

  // NotificationHelperImpl:
  void InitSystemNotifications(base::OnceClosure callback) override;
  bool CanShowNotifications() override;
  bool CanShowSystemNotificationsWhileBrowserIsBackgrounded() const override;
  bool ShowOnboardingNotification() override;

 protected:
  friend class NotificationHelper;
  NotificationHelperImplWin();

  void InitSystemNotificationsCallback(
      base::OnceClosure callback,
      Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotifier>
          toast_notifier);

  bool IsFocusAssistEnabled() const;

  bool IsNotificationsEnabled();

  Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotifier>
      toast_notifier_;
  base::WeakPtrFactory<NotificationHelperImplWin> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_NOTIFICATION_HELPER_NOTIFICATION_HELPER_IMPL_WIN_H_
