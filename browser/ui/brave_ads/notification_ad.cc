/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/notification_ad.h"

#include <vector>

#include "base/strings/string_util.h"

namespace brave_ads {

NotificationAd::NotificationAd(const std::string& id,
                               const std::u16string& title,
                               const std::u16string& body,
                               scoped_refptr<NotificationAdDelegate> delegate)
    : id_(id), title_(title), body_(body), delegate_(std::move(delegate)) {}

NotificationAd::NotificationAd(scoped_refptr<NotificationAdDelegate> delegate,
                               const NotificationAd& other)
    : NotificationAd(other) {
  delegate_ = delegate;
}

NotificationAd::NotificationAd(const std::string& id,
                               const NotificationAd& other)
    : NotificationAd(other) {
  id_ = id;
}

NotificationAd::NotificationAd(const NotificationAd& other) = default;

NotificationAd& NotificationAd::operator=(const NotificationAd& other) =
    default;

NotificationAd::~NotificationAd() = default;

std::u16string NotificationAd::accessible_name() const {
  std::vector<std::u16string> accessible_lines;

  if (!title_.empty()) {
    accessible_lines.push_back(title_);
  }

  if (!body_.empty()) {
    accessible_lines.push_back(body_);
  }

  return base::JoinString(accessible_lines, u"\n");
}

}  // namespace brave_ads
