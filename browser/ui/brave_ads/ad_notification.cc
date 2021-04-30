/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/ad_notification.h"

#include <vector>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

namespace brave_ads {

AdNotification::AdNotification(const std::string& id,
                               const base::string16& title,
                               const base::string16& body,
                               scoped_refptr<AdNotificationDelegate> delegate)
    : id_(id), title_(title), body_(body), delegate_(std::move(delegate)) {}

AdNotification::AdNotification(scoped_refptr<AdNotificationDelegate> delegate,
                               const AdNotification& other)
    : AdNotification(other) {
  delegate_ = delegate;
}

AdNotification::AdNotification(const std::string& id,
                               const AdNotification& other)
    : AdNotification(other) {
  id_ = id;
}

AdNotification::AdNotification(const AdNotification& other) = default;

AdNotification& AdNotification::operator=(const AdNotification& other) =
    default;

AdNotification::~AdNotification() = default;

base::string16 AdNotification::accessible_name() const {
  std::vector<base::string16> accessible_lines;

  if (!title_.empty()) {
    accessible_lines.push_back(title_);
  }

  if (!body_.empty()) {
    accessible_lines.push_back(body_);
  }

  return base::JoinString(accessible_lines, base::ASCIIToUTF16("\n"));
}

}  // namespace brave_ads
