// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_PERMISSION_REQUEST_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_PERMISSION_REQUEST_H_

#include "components/permissions/permission_request.h"

// Represents the PSST permission request
class PsstPermissionRequest : public permissions::PermissionRequest {
 public:
  using PermissionRrequestCallback =
      base::OnceCallback<void(const bool is_accepted)>;

  explicit PsstPermissionRequest(const GURL& requesting_origin,
                                 PermissionRrequestCallback callback);
  PsstPermissionRequest(const PsstPermissionRequest&) = delete;
  PsstPermissionRequest& operator=(const PsstPermissionRequest&) = delete;
  ~PsstPermissionRequest() override;

 private:
  void PermissionDecided(
      PermissionDecision decision,
      bool is_final_decision,
      const permissions::PermissionRequestData& request_data);
  PermissionRrequestCallback callback_;
};

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_PERMISSION_REQUEST_H_
