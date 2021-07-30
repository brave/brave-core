/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_context_base.h"
#include "components/permissions/permissions_client.h"

#define PermissionContextBase PermissionContextBase_ChromiumImpl
#define CanBypassEmbeddingOriginCheck(REQUESTING_ORIGIN, EMBEDDING_ORIGIN) \
  BraveCanBypassEmbeddingOriginCheck(REQUESTING_ORIGIN, EMBEDDING_ORIGIN,  \
                                     content_settings_type_)
#include "../../../../components/permissions/permission_context_base.cc"
#undef PermissionContextBase
#undef CanBypassEmbeddingOriginCheck

#include "brave/components/permissions/permission_lifetime_manager.h"

namespace {

bool IsGroupedPermissionType(ContentSettingsType type) {
  return type == ContentSettingsType::BRAVE_ETHEREUM;
}

}  // namespace

namespace permissions {

PermissionContextBase::PermissionContextBase(
    content::BrowserContext* browser_context,
    ContentSettingsType content_settings_type,
    blink::mojom::PermissionsPolicyFeature permissions_policy_feature)
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
    const PermissionRequestID& id,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    BrowserPermissionCallback callback,
    ContentSetting content_setting,
    bool is_one_time) {
  if (permission_lifetime_manager_factory_) {
    const auto request_it = pending_requests_.find(id.ToString());
    if (request_it != pending_requests_.end()) {
      const PermissionRequest* permission_request = request_it->second.get();
      DCHECK(permission_request);
      if (auto* permission_lifetime_manager =
              permission_lifetime_manager_factory_.Run(browser_context_)) {
        permission_lifetime_manager->PermissionDecided(
            *permission_request, requesting_origin, embedding_origin,
            content_setting, is_one_time);
      }
    }
  }
  PermissionContextBase_ChromiumImpl::PermissionDecided(
      id, requesting_origin, embedding_origin, std::move(callback),
      content_setting, is_one_time);
}

void PermissionContextBase::DecidePermission(
    content::WebContents* web_contents,
    const PermissionRequestID& id,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    bool user_gesture,
    BrowserPermissionCallback callback) {
  PermissionContextBase_ChromiumImpl::DecidePermission(
      web_contents, id, requesting_origin, embedding_origin, user_gesture,
      std::move(callback));

  if (!IsGroupedPermissionType(content_settings_type()))
    return;

  // Move added pending request from pending_requests_ to
  // pending_grouped_requests_, because otherwise Chromium will replace this
  // pending_request with next sub-request because it does not expect multiple
  // sub-requests in the same type for one RequestPermissions request.
  auto pending_request = pending_requests_.find(id.ToString());
  DCHECK(pending_request != pending_requests_.end());

  auto it = pending_grouped_requests_.find(id.ToString());
  if (it == pending_grouped_requests_.end()) {
    it = pending_grouped_requests_
             .insert(std::make_pair(
                 id.ToString(), std::make_unique<GroupedPermissionRequests>()))
             .first;
  }
  it->second->AddRequest(std::move(pending_request->second));

  pending_requests_.erase(pending_request);
}

void PermissionContextBase::CleanUpRequest(const PermissionRequestID& id) {
  if (!IsGroupedPermissionType(content_settings_type())) {
    PermissionContextBase_ChromiumImpl::CleanUpRequest(id);
    return;
  }

  // A sub-request is done, increase finish count. If all sub-requests are
  // done, remove all sub-requests.
  auto requests = pending_grouped_requests_.find(id.ToString());
  DCHECK(requests != pending_grouped_requests_.end());
  requests->second->RequestFinished();
  if (requests->second->IsDone()) {
    pending_grouped_requests_.erase(id.ToString());
  }
}

PermissionContextBase::GroupedPermissionRequests::GroupedPermissionRequests() =
    default;
PermissionContextBase::GroupedPermissionRequests::~GroupedPermissionRequests() =
    default;

bool PermissionContextBase::GroupedPermissionRequests::IsDone() const {
  return finished_request_count_ == requests_.size();
}

void PermissionContextBase::GroupedPermissionRequests::AddRequest(
    std::unique_ptr<PermissionRequest> request) {
  requests_.push_back(std::move(request));
}

void PermissionContextBase::GroupedPermissionRequests::RequestFinished() {
  finished_request_count_++;
}

bool PermissionContextBase::IsPendingGroupedRequestsEmptyForTesting() {
  return pending_grouped_requests_.empty();
}

}  // namespace permissions
