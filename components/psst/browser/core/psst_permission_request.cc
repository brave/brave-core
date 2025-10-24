// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_permission_request.h"

#include "components/permissions/resolvers/content_setting_permission_resolver.h"

PsstPermissionRequest::PsstPermissionRequest(
    const GURL& requesting_origin,
    PermissionRrequestCallback callback)
    : PermissionRequest(
          std::make_unique<permissions::PermissionRequestData>(
              std::make_unique<permissions::ContentSettingPermissionResolver>(
                  permissions::RequestType::kBravePsst),
              false,
              requesting_origin),
          base::BindRepeating(&PsstPermissionRequest::PermissionDecided,
                              base::Unretained(this))),
      callback_(std::move(callback)) {}

PsstPermissionRequest::~PsstPermissionRequest() = default;

void PsstPermissionRequest::PermissionDecided(
    PermissionDecision decision,
    bool is_final_decision,
    const permissions::PermissionRequestData& request_data) {
  if (callback_) {
    std::move(callback_).Run(decision == PermissionDecision::kAllow ||
                             decision == PermissionDecision::kAllowThisTime);
  }
}
