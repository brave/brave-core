/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULES_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULES_BASE_H_

namespace brave_ads {

class PermissionRulesBase {
 public:
  PermissionRulesBase(const PermissionRulesBase&) = delete;
  PermissionRulesBase& operator=(const PermissionRulesBase&) = delete;

  virtual ~PermissionRulesBase();

 protected:
  PermissionRulesBase();

  static bool HasPermission();
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULES_BASE_H_
