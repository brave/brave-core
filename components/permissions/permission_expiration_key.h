/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_EXPIRATION_KEY_H_
#define BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_EXPIRATION_KEY_H_

#include <string>

#include "base/time/time.h"

namespace permissions {

// Stores permission expiration key (an expiration time or a bound domain).
class PermissionExpirationKey {
 public:
  explicit PermissionExpirationKey(base::Time time);
  explicit PermissionExpirationKey(std::string domain);
  PermissionExpirationKey(const PermissionExpirationKey&);
  PermissionExpirationKey& operator=(const PermissionExpirationKey&);
  PermissionExpirationKey(PermissionExpirationKey&&) noexcept;
  PermissionExpirationKey& operator=(PermissionExpirationKey&&) noexcept;
  ~PermissionExpirationKey();

  static PermissionExpirationKey FromString(const std::string& key_str);
  std::string ToString() const;

  bool operator<(const PermissionExpirationKey&) const;
  bool operator==(const PermissionExpirationKey&) const;

  bool IsTimeKey() const;
  const base::Time& time() const { return time_; }
  const std::string& domain() const { return domain_; }

 private:
  base::Time time_;
  std::string domain_;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_EXPIRATION_KEY_H_
