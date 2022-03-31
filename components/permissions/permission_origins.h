/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_ORIGINS_H_
#define BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_ORIGINS_H_

#include <string>

#include "components/content_settings/core/common/content_settings.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace permissions {

// Stores permission origins without duplicating |embedding_origin| if it's same
// as |requesting_origin|.
class PermissionOrigins {
 public:
  PermissionOrigins(const GURL& requesting_origin,
                    const GURL& embedding_origin,
                    ContentSetting content_setting);
  PermissionOrigins(const std::string* requesting_origin,
                    const std::string* embedding_origin,
                    int content_setting);
  PermissionOrigins(const PermissionOrigins&);
  PermissionOrigins& operator=(const PermissionOrigins&);
  PermissionOrigins(PermissionOrigins&&) noexcept;
  PermissionOrigins& operator=(PermissionOrigins&&) noexcept;
  ~PermissionOrigins();

  bool operator==(const PermissionOrigins& rhs) const;

  const GURL& requesting_origin() const { return requesting_origin_; }
  const GURL& embedding_origin() const {
    return embedding_origin_ ? *embedding_origin_ : requesting_origin_;
  }
  ContentSetting content_setting() const { return content_setting_; }

 private:
  GURL requesting_origin_;
  absl::optional<GURL> embedding_origin_;
  ContentSetting content_setting_;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_ORIGINS_H_
