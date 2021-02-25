/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_origins.h"

namespace permissions {

PermissionOrigins::PermissionOrigins(const GURL& requesting_origin,
                                     const GURL& embedding_origin)
    : requesting_origin_(requesting_origin),
      embedding_origin_(embedding_origin) {}

PermissionOrigins::PermissionOrigins(const std::string* requesting_origin,
                                     const std::string* embedding_origin)
    : requesting_origin_(GURL(*requesting_origin)),
      embedding_origin_(embedding_origin
                            ? base::make_optional<GURL>(*embedding_origin)
                            : base::nullopt) {}

PermissionOrigins::PermissionOrigins(const PermissionOrigins&) = default;
PermissionOrigins& PermissionOrigins::operator=(const PermissionOrigins&) =
    default;
PermissionOrigins::PermissionOrigins(PermissionOrigins&&) noexcept = default;
PermissionOrigins& PermissionOrigins::operator=(PermissionOrigins&&) noexcept =
    default;
PermissionOrigins::~PermissionOrigins() = default;

bool PermissionOrigins::operator==(const PermissionOrigins& rhs) const {
  auto tie = [](const PermissionOrigins& obj) {
    return std::tie(obj.requesting_origin_, obj.embedding_origin_);
  };
  return tie(*this) == tie(rhs);
}

}  // namespace permissions
