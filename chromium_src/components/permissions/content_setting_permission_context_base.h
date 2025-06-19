/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_CONTENT_SETTING_PERMISSION_CONTEXT_BASE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_CONTENT_SETTING_PERMISSION_CONTEXT_BASE_H_

#include "base/functional/callback.h"
#include "components/permissions/permission_context_base.h"

namespace permissions {
class ContentSettingPermissionContextBase;
using ContentSettingPermissionContextBase_BraveImpl =
    ContentSettingPermissionContextBase;
class PermissionLifetimeManager;
}  // namespace permissions

#define ContentSettingPermissionContextBase \
  ContentSettingPermissionContextBase_ChromiumImpl
#define BRAVE_CONTENT_SETTING_PERMISSION_CONTEXT_BASE_ \
  friend ContentSettingPermissionContextBase_BraveImpl;
#include "src/components/permissions/content_setting_permission_context_base.h"  // IWYU pragma: export

#undef BRAVE_CONTENT_SETTING_PERMISSION_CONTEXT_BASE_
#undef ContentSettingPermissionContextBase

#include <map>

namespace permissions {

class ContentSettingPermissionContextBase
    : public ContentSettingPermissionContextBase_ChromiumImpl {
 public:
  ContentSettingPermissionContextBase(
      content::BrowserContext* browser_context,
      ContentSettingsType content_settings_type,
      network::mojom::PermissionsPolicyFeature permissions_policy_feature);

  ~ContentSettingPermissionContextBase() override;

  void SetPermissionLifetimeManagerFactory(
      const base::RepeatingCallback<
          PermissionLifetimeManager*(content::BrowserContext*)>& factory);

  void DecidePermission(
      std::unique_ptr<permissions::PermissionRequestData> request_data,
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

    using GroupedRequests = std::vector<
        std::pair<base::WeakPtr<PermissionRequest>, BrowserPermissionCallback>>;

    bool IsDone() const;
    void AddRequest(std::pair<base::WeakPtr<PermissionRequest>,
                              BrowserPermissionCallback> request);
    BrowserPermissionCallback GetNextCallback();
    void RequestFinished();

    const GroupedRequests& Requests() const { return requests_; }

   private:
    GroupedRequests requests_;
    size_t finished_request_count_ = 0;
    size_t next_callback_index_ = 0;
  };

  void PermissionDecided(PermissionDecision decision,
                         bool is_final_decision,
                         const PermissionRequestData& request_data) override;
  void CleanUpRequest(content::WebContents* web_contents,
                      const PermissionRequestID& id,
                      bool embedded_permission_element_initiated) override;

  base::RepeatingCallback<PermissionLifetimeManager*(content::BrowserContext*)>
      permission_lifetime_manager_factory_;

  std::map<std::string, std::unique_ptr<GroupedPermissionRequests>>
      pending_grouped_requests_;
};

}  // namespace permissions

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_CONTENT_SETTING_PERMISSION_CONTEXT_BASE_H_
