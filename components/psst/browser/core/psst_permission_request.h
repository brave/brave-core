// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_PERMISSION_REQUEST_H_
#define BRAVE_BROWSER_PSST_PSST_PERMISSION_REQUEST_H_

#include "components/permissions/permission_request.h"


class PsstPermissionRequest : public permissions::PermissionRequest {
 public:
  explicit PsstPermissionRequest(const GURL& requesting_origin);
  PsstPermissionRequest(const PsstPermissionRequest&) = delete;
  PsstPermissionRequest& operator=(const PsstPermissionRequest&) = delete;
  ~PsstPermissionRequest() override;

 private:
    void PermissionDecided(
      PermissionDecision decision,
      bool is_final_decision,
      const permissions::PermissionRequestData& request_data);

};

#endif  // BRAVE_BROWSER_PSST_PSST_PERMISSION_REQUEST_H_