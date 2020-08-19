/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ui/brave_custom_notification/public/cpp/notification_delegate.h"

#include "base/bind.h"
#include "base/logging.h"

namespace brave_custom_notification {

ThunkNotificationDelegate::ThunkNotificationDelegate(
    base::WeakPtr<NotificationObserver> impl)
    : impl_(impl) {}

void ThunkNotificationDelegate::Close(bool by_user) {
  if (impl_)
    impl_->Close(by_user);
}

void ThunkNotificationDelegate::Click(
    const base::Optional<int>& button_index,
    const base::Optional<base::string16>& reply) {
  if (impl_)
    impl_->Click(button_index, reply);
}

void ThunkNotificationDelegate::SettingsClick() {
  if (impl_)
    impl_->SettingsClick();
}

void ThunkNotificationDelegate::DisableNotification() {
  if (impl_)
    impl_->DisableNotification();
}

ThunkNotificationDelegate::~ThunkNotificationDelegate() = default;

HandleNotificationClickDelegate::HandleNotificationClickDelegate(
    const base::RepeatingClosure& callback) {
  SetCallback(callback);
}

HandleNotificationClickDelegate::HandleNotificationClickDelegate(
    const ButtonClickCallback& callback)
    : callback_(callback) {}

void HandleNotificationClickDelegate::SetCallback(
    const ButtonClickCallback& callback) {
  callback_ = callback;
}

void HandleNotificationClickDelegate::SetCallback(
    const base::RepeatingClosure& closure) {
  if (!closure.is_null()) {
    // Create a callback that consumes and ignores the button index parameter,
    // and just runs the provided closure.
    callback_ = base::BindRepeating(
        [](const base::RepeatingClosure& closure,
           base::Optional<int> button_index) {
          DCHECK(!button_index);
          closure.Run();
        },
        closure);
  }
}

HandleNotificationClickDelegate::~HandleNotificationClickDelegate() {}

void HandleNotificationClickDelegate::Click(
    const base::Optional<int>& button_index,
    const base::Optional<base::string16>& reply) {
  if (!callback_.is_null())
    callback_.Run(button_index);
}

}  // namespace brave_custom_notification
