/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_context_base.h"

#define PermissionContextBase PermissionContextBase_ChromiumImpl
#include "../../../../components/permissions/permission_context_base.cc"
#undef PermissionContextBase

#include "brave/components/permissions/permission_lifetime_manager.h"

namespace permissions {

void PermissionContextBase::SetPermissionLifetimeManager(
    PermissionLifetimeManager* lifetime_manager) {
  permission_lifetime_manager_ = lifetime_manager;
}

void PermissionContextBase::PermissionDecided(
    const PermissionRequestID& id,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    BrowserPermissionCallback callback,
    ContentSetting content_setting,
    bool is_one_time) {
  if (permission_lifetime_manager_) {
    const auto request_it = pending_requests_.find(id.ToString());
    if (request_it != pending_requests_.end()) {
      const PermissionRequest* permission_request = request_it->second.get();
      DCHECK(permission_request);
      permission_lifetime_manager_->PermissionDecided(
          *permission_request, requesting_origin, embedding_origin,
          content_setting, is_one_time);
    }
  }
  PermissionContextBase_ChromiumImpl::PermissionDecided(
      id, requesting_origin, embedding_origin, std::move(callback),
      content_setting, is_one_time);
}

}  // namespace permissions
