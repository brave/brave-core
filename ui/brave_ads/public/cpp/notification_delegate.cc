// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/ui/brave_ads/public/cpp/notification_delegate.h"

#include "base/bind.h"
#include "base/logging.h"

namespace brave_ads {

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
    const base::Optional<std::u16string>& reply) {
  if (!callback_.is_null())
    callback_.Run(button_index);
}

}  // namespace brave_ads
