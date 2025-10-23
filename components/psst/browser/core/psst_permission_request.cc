// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_permission_request.h"

#include "components/permissions/resolvers/content_setting_permission_resolver.h"

PsstPermissionRequest::PsstPermissionRequest(const GURL& requesting_origin)
    : PermissionRequest(
          std::make_unique<permissions::PermissionRequestData>(
              std::make_unique<permissions::ContentSettingPermissionResolver>(
                  permissions::RequestType::kBravePsst),
              false,
              requesting_origin),
          base::BindRepeating(&PsstPermissionRequest::PermissionDecided,
                              base::Unretained(this))) {
  LOG(INFO) << "[PSST] PsstPermissionRequest requesting_origin: " << requesting_origin;
}

PsstPermissionRequest::~PsstPermissionRequest() = default;

void PsstPermissionRequest::PermissionDecided(
    PermissionDecision decision,
    bool is_final_decision,
    const permissions::PermissionRequestData& request_data) {
LOG(INFO) << "[PSST] PsstPermissionRequest::PermissionDecided called with decision: " << static_cast<int>(decision)
<< " requesting_origin: " << request_data.requesting_origin.spec()
<< " embedding_origin: " << request_data.embedding_origin.spec()
<< " request_type: " << (request_data.request_type.has_value() ? std::to_string(static_cast<int>(request_data.request_type.value())) : "null")
;
}
