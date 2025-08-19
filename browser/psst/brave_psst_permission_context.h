/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PSST_BRAVE_PSST_PERMISSION_CONTEXT_H_
#define BRAVE_BROWSER_PSST_BRAVE_PSST_PERMISSION_CONTEXT_H_

#include "brave/components/psst/common/psst_common.h"
#include "components/permissions/object_permission_context_base.h"

namespace psst {

class BravePsstPermissionContext
    : public permissions::ObjectPermissionContextBase {
 public:
  explicit BravePsstPermissionContext(
      HostContentSettingsMap* host_content_settings_map);
  BravePsstPermissionContext(const BravePsstPermissionContext&) = delete;
  BravePsstPermissionContext& operator=(const BravePsstPermissionContext&) =
      delete;
  ~BravePsstPermissionContext() override;

  void CreateOrUpdate(const url::Origin& origin,
                      const PsstPermissionInfo& permission_info);
  void Revoke(const url::Origin& origin, const std::string& user_id);

  std::optional<PsstPermissionInfo> GetPsstPermissionInfo(
      const url::Origin& origin,
      const std::string& user_id);

 private:
  // permissions::ObjectPermissionContextBase implementation:
  std::string GetKeyForObject(const base::Value::Dict& object) override;
  bool IsValidObject(const base::Value::Dict& object) override;
  std::u16string GetObjectDisplayName(const base::Value::Dict& object) override;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_BRAVE_PSST_PERMISSION_CONTEXT_H_
