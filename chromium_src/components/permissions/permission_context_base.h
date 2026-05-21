/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_CONTEXT_BASE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_CONTEXT_BASE_H_

#include <components/permissions/permission_context_base.h>  // IWYU pragma: export

namespace permissions {

class PermissionLifetimeManager;

class PermissionContextBase : public chromium_impl::PermissionContextBase {
 public:
  PermissionContextBase(
      content::BrowserContext* browser_context,
      ContentSettingsType content_settings_type,
      network::mojom::PermissionsPolicyFeature permissions_policy_feature);

  ~PermissionContextBase() override;

  void SetPermissionLifetimeManagerFactory(
      const base::RepeatingCallback<
          PermissionLifetimeManager*(content::BrowserContext*)>& factory);

 private:
  void PermissionDecided(const permissions::PermissionPromptDecision& decision,
                         const PermissionRequestData& request_data) override;

  base::RepeatingCallback<PermissionLifetimeManager*(content::BrowserContext*)>
      permission_lifetime_manager_factory_;
};

}  // namespace permissions

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_CONTEXT_BASE_H_
