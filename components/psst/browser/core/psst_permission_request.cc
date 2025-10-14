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
          base::DoNothing()) {}

PsstPermissionRequest::~PsstPermissionRequest() = default;
