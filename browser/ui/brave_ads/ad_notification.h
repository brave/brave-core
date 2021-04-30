/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_H_
#define BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_H_

#include <string>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "brave/browser/ui/brave_ads/ad_notification_delegate.h"

namespace brave_ads {

class AdNotification {
 public:
  // Create a new ad notification with an |id|, |title| text and |body| text.
  // |delegate| will influence the behaviour of this ad notification and
  // receives events on its behalf. The delegate may be omitted
  AdNotification(const std::string& id,
                 const base::string16& title,
                 const base::string16& body,
                 scoped_refptr<AdNotificationDelegate> delegate);

  // Creates a copy of the |other| ad notification. The delegate, if any, will
  // be identical for both ad notification instances. The |id| of the ad
  // notification will be replaced by the given value
  AdNotification(const std::string& id, const AdNotification& other);

  // Creates a copy of the |other| ad notification. The delegate will be
  // replaced by |delegate|
  AdNotification(scoped_refptr<AdNotificationDelegate> delegate,
                 const AdNotification& other);

  // Creates a copy of the |other| ad notification. The delegate, if any, will
  // be identical for both ad notification instances
  AdNotification(const AdNotification& other);

  AdNotification& operator=(const AdNotification& other);

  virtual ~AdNotification();

  const std::string& id() const { return id_; }

  const base::string16& title() const { return title_; }
  void set_title(const base::string16& title) { title_ = title; }

  const base::string16& body() const { return body_; }
  void set_body(const base::string16& body) { body_ = body; }

  base::string16 accessible_name() const;

  AdNotificationDelegate* delegate() const { return delegate_.get(); }

  void set_delegate(scoped_refptr<AdNotificationDelegate> delegate) {
    DCHECK(!delegate_);
    delegate_ = std::move(delegate);
  }

 protected:
  std::string id_;

  base::string16 title_;
  base::string16 body_;

 private:
  // A proxy object that allows access back to the JavaScript object that
  // represents the notification, for firing events
  scoped_refptr<AdNotificationDelegate> delegate_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_H_
