/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_context_base.h"

#include "base/check.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/permissions/permission_lifetime_manager.h"
#include "components/permissions/permissions_client.h"

#define CanBypassEmbeddingOriginCheck(REQUESTING_ORIGIN, EMBEDDING_ORIGIN) \
  BraveCanBypassEmbeddingOriginCheck(REQUESTING_ORIGIN, EMBEDDING_ORIGIN,  \
                                     content_settings_type_)
#define PermissionContextBase PermissionContextBase_ChromiumImpl
#include <components/permissions/permission_context_base.cc>
#undef CanBypassEmbeddingOriginCheck
#undef PermissionContextBase

namespace permissions {

PermissionContextBase::PermissionContextBase(
    content::BrowserContext* browser_context,
    ContentSettingsType content_settings_type,
    network::mojom::PermissionsPolicyFeature permissions_policy_feature)
    : PermissionContextBase_ChromiumImpl(browser_context,
                                         content_settings_type,
                                         permissions_policy_feature) {}

PermissionContextBase::~PermissionContextBase() = default;

void PermissionContextBase::SetPermissionLifetimeManagerFactory(
    const base::RepeatingCallback<
        PermissionLifetimeManager*(content::BrowserContext*)>& factory) {
  permission_lifetime_manager_factory_ = factory;
}

void PermissionContextBase::PermissionDecided(
    PermissionDecision decision,
    bool is_final_decision,
    const PermissionRequestData& request_data) {
  if (permission_lifetime_manager_factory_) {
    const auto request_it = pending_requests_.find(request_data.id.ToString());
    if (request_it != pending_requests_.end()) {
      const PermissionRequest* permission_request =
          request_it->second.first.get();
      CHECK(permission_request);
      if (auto* permission_lifetime_manager =
              permission_lifetime_manager_factory_.Run(browser_context_)) {
        permission_lifetime_manager->PermissionDecided(
            *permission_request, request_data.requesting_origin,
            request_data.embedding_origin, decision);
      }
    }
  }

  PermissionContextBase_ChromiumImpl::PermissionDecided(
      decision, is_final_decision, request_data);
}

}  // namespace permissions
