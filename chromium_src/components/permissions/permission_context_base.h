/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_CONTEXT_BASE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_CONTEXT_BASE_H_

#include "base/functional/callback.h"

namespace permissions {
class PermissionContextBase;
using PermissionContextBase_BraveImpl = PermissionContextBase;
class PermissionLifetimeManager;
}  // namespace permissions

#define PermissionContextBase PermissionContextBase_ChromiumImpl
#define PermissionDecided virtual PermissionDecided
#define BRAVE_PERMISSION_CONTEXT_BASE_H_              \
  friend PermissionContextBase_BraveImpl;             \
                                                      \
 protected:                                           \
  base::RepeatingCallback<PermissionLifetimeManager*( \
      content::BrowserContext*)>                      \
      permission_lifetime_manager_factory_;
#define CleanUpRequest virtual CleanUpRequest

#include "src/components/permissions/permission_context_base.h"  // IWYU pragma: export

#undef BRAVE_PERMISSION_CONTEXT_BASE_H_
#undef CleanUpRequest
#undef PermissionDecided
#undef PermissionContextBase

#include <map>

namespace permissions {

class PermissionContextBase : public PermissionContextBase_ChromiumImpl {
 public:
  PermissionContextBase(
      content::BrowserContext* browser_context,
      ContentSettingsType content_settings_type,
      blink::mojom::PermissionsPolicyFeature permissions_policy_feature);

  ~PermissionContextBase() override;

  void SetPermissionLifetimeManagerFactory(
      const base::RepeatingCallback<
          PermissionLifetimeManager*(content::BrowserContext*)>& factory);

  void DecidePermission(permissions::PermissionRequestData request_data,
                        BrowserPermissionCallback callback) override;

  bool IsPendingGroupedRequestsEmptyForTesting();

 private:
  /**
   * This class is map to one PermissionManager::RequestPermissions request,
   * sub-requests will be kept in requests_.
   * Chromium does not expect multiple sub-requests for a same permission type,
   * this class is created to support tracking multiple sub-requests
   * for each RequestPermissions request. It will clear all pending
   * sub-requests for one RequestPermissions request after all of its
   * sub-requests are finished.
   */
  class GroupedPermissionRequests {
   public:
    GroupedPermissionRequests();
    ~GroupedPermissionRequests();

    using GroupedRequests =
        std::vector<std::pair<std::unique_ptr<PermissionRequest>,
                              BrowserPermissionCallback>>;

    bool IsDone() const;
    void AddRequest(std::pair<std::unique_ptr<PermissionRequest>,
                              BrowserPermissionCallback> request);
    BrowserPermissionCallback GetNextCallback();
    void RequestFinished();

    const GroupedRequests& Requests() const { return requests_; }

   private:
    GroupedRequests requests_;
    size_t finished_request_count_ = 0;
    size_t next_callback_index_ = 0;
  };

  void PermissionDecided(const PermissionRequestID& id,
                         const GURL& requesting_origin,
                         const GURL& embedding_origin,
                         ContentSetting content_setting,
                         bool is_one_time,
                         bool is_final_decision) override;
  void CleanUpRequest(const PermissionRequestID& id,
                      bool embedded_permission_element_initiated) override;

  std::map<std::string, std::unique_ptr<GroupedPermissionRequests>>
      pending_grouped_requests_;
};

}  // namespace permissions

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_CONTEXT_BASE_H_
