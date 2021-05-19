// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UI_BRAVE_ADS_PUBLIC_CPP_NOTIFICATION_DELEGATE_H_
#define BRAVE_UI_BRAVE_ADS_PUBLIC_CPP_NOTIFICATION_DELEGATE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"

namespace brave_ads {

// Handles actions performed on a notification.
class NotificationObserver {
 public:
  // Called when the desktop notification is closed. If closed by a user
  // explicitly (as opposed to timeout/script), |by_user| should be true.
  virtual void Close(bool by_user) {}

  // Called when a desktop notification is clicked. |button_index| is filled in
  // if a button was clicked (as opposed to the body of the notification) while
  // |reply| is filled in if there was an input field associated with the
  // button.
  virtual void Click(const base::Optional<int>& button_index,
                     const base::Optional<std::u16string>& reply) {}
};

// Ref counted version of NotificationObserver, required to satisfy
// brave_ads::Notification::delegate_.
class NotificationDelegate
    : public NotificationObserver,
      public base::RefCountedThreadSafe<NotificationDelegate> {
 protected:
  virtual ~NotificationDelegate() = default;

 private:
  friend class base::RefCountedThreadSafe<NotificationDelegate>;
};

// A simple notification delegate which invokes the passed closure when the body
// or a button is clicked.
class HandleNotificationClickDelegate : public NotificationDelegate {
 public:
  // The parameter is the index of the button that was clicked, or nullopt if
  // the body was clicked.
  using ButtonClickCallback =
      base::RepeatingCallback<void(base::Optional<int>)>;

  // Creates a delegate that handles clicks on a button or on the body.
  explicit HandleNotificationClickDelegate(const ButtonClickCallback& callback);

  // Creates a delegate that only handles clicks on the body of the
  // notification.
  explicit HandleNotificationClickDelegate(
      const base::RepeatingClosure& closure);

  // Overrides the callback with one that handles clicks on a button or on the
  // body.
  void SetCallback(const ButtonClickCallback& callback);

  // Overrides the callback with one that only handles clicks on the body of the
  // notification.
  void SetCallback(const base::RepeatingClosure& closure);

  // NotificationDelegate overrides:
  void Click(const base::Optional<int>& button_index,
             const base::Optional<std::u16string>& reply) override;

 protected:
  ~HandleNotificationClickDelegate() override;

 private:
  ButtonClickCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(HandleNotificationClickDelegate);
};

}  // namespace brave_ads

#endif  // BRAVE_UI_BRAVE_ADS_PUBLIC_CPP_NOTIFICATION_DELEGATE_H_
