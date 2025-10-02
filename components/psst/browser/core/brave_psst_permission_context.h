/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_BRAVE_PSST_PERMISSION_CONTEXT_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_BRAVE_PSST_PERMISSION_CONTEXT_H_

#include "brave/components/psst/common/psst_permission_schema.h"
#include "brave/components/psst/common/psst_ui_common.mojom-forward.h"
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

  void GrantPermission(const url::Origin& origin,
                       ConsentStatus consent_status,
                       int script_version,
                       std::string_view user_id,
                       base::Value::List urls_to_skip);
  void GrantPermission(const url::Origin& origin,
                       PsstPermissionInfo permission_info);
  bool HasPermission(const url::Origin& origin, std::string_view user_id);
  void RevokePermission(const url::Origin& origin, std::string_view user_id);

  std::optional<PsstPermissionInfo> GetPsstPermissionInfo(
      const url::Origin& origin,
      std::string_view user_id);

 private:
  // permissions::ObjectPermissionContextBase implementation:
  std::string GetKeyForObject(const base::Value::Dict& object) override;
  bool IsValidObject(const base::Value::Dict& object) override;
  std::u16string GetObjectDisplayName(const base::Value::Dict& object) override;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_BRAVE_PSST_PERMISSION_CONTEXT_H_
