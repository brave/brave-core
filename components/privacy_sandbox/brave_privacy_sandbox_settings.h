/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_
#define BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/privacy_sandbox/privacy_sandbox_settings.h"
#include "components/privacy_sandbox/tpcd_experiment_eligibility.h"

class HostContentSettingsMap;
class PrefService;

namespace content_settings {
class CookieSettings;
}  // namespace content_settings

namespace privacy_sandbox {
class CanonicalTopic;
class TrackingProtectionSettings;
}  // namespace privacy_sandbox

class BravePrivacySandboxSettings
    : public privacy_sandbox::PrivacySandboxSettings {
 public:
  BravePrivacySandboxSettings(
      std::unique_ptr<Delegate> delegate,
      HostContentSettingsMap* host_content_settings_map,
      content_settings::CookieSettings* cookie_settings,
      privacy_sandbox::TrackingProtectionSettings* tracking_protection_settings,
      PrefService* pref_service);
  ~BravePrivacySandboxSettings() override;

  // PrivacySandboxSettings:
  bool IsTopicsAllowed() const override;
  bool IsTopicsAllowedForContext(
      const url::Origin& top_frame_origin,
      const GURL& url,
      content::RenderFrameHost* console_frame = nullptr) const override;
  bool IsTopicAllowed(const privacy_sandbox::CanonicalTopic& topic) override;
  void SetTopicAllowed(const privacy_sandbox::CanonicalTopic& topic,
                       bool allowed) override;
  bool IsTopicPrioritized(
      const privacy_sandbox::CanonicalTopic& topic) override;
  void ClearTopicSettings(base::Time start_time, base::Time end_time) override;
  base::Time TopicsDataAccessibleSince() const override;
  bool IsAttributionReportingEverAllowed() const override;
  bool IsAttributionReportingAllowed(
      const url::Origin& top_frame_origin,
      const url::Origin& reporting_origin,
      content::RenderFrameHost* console_frame = nullptr) const override;
  bool MaySendAttributionReport(
      const url::Origin& source_origin,
      const url::Origin& destination_origin,
      const url::Origin& reporting_origin,
      content::RenderFrameHost* console_frame = nullptr) const override;
  bool IsAttributionReportingTransitionalDebuggingAllowed(
      const url::Origin& top_frame_origin,
      const url::Origin& reporting_origin,
      bool& can_bypass) const override;
  void SetFledgeJoiningAllowed(const std::string& top_frame_etld_plus1,
                               bool allowed) override;
  void ClearFledgeJoiningAllowedSettings(base::Time start_time,
                                         base::Time end_time) override;
  bool IsFledgeAllowed(
      const url::Origin& top_frame_origin,
      const url::Origin& auction_party,
      content::InterestGroupApiOperation interest_group_api_operation,
      content::RenderFrameHost* console_frame = nullptr) const override;
  bool IsEventReportingDestinationAttested(
      const url::Origin& destination_origin,
      privacy_sandbox::PrivacySandboxAttestationsGatedAPI invoking_api)
      const override;
  bool IsSharedStorageAllowed(
      const url::Origin& top_frame_origin,
      const url::Origin& accessing_origin,
      std::string* out_debug_message,
      content::RenderFrameHost* console_frame,
      bool* out_block_is_site_setting_specific) const override;
  bool IsSharedStorageSelectURLAllowed(
      const url::Origin& top_frame_origin,
      const url::Origin& accessing_origin,
      std::string* out_debug_message,
      bool* out_block_is_site_setting_specific) const override;
  bool IsFencedStorageReadAllowed(
      const url::Origin& top_frame_origin,
      const url::Origin& accessing_origin,
      content::RenderFrameHost* console_frame) const override;
  bool IsPrivateAggregationAllowed(
      const url::Origin& top_frame_origin,
      const url::Origin& reporting_origin,
      bool* out_block_is_site_setting_specific) const override;
  bool IsPrivateAggregationDebugModeAllowed(
      const url::Origin& top_frame_origin,
      const url::Origin& reporting_origin) const override;
  privacy_sandbox::TpcdExperimentEligibility
  GetCookieDeprecationExperimentCurrentEligibility() const override;

  bool IsCookieDeprecationLabelAllowed() const override;
  bool IsCookieDeprecationLabelAllowedForContext(
      const url::Origin& top_frame_origin,
      const url::Origin& context_origin) const override;
  void SetAllPrivacySandboxAllowedForTesting() override;
  void SetTopicsBlockedForTesting() override;
  bool IsPrivacySandboxRestricted() const override;
  bool IsPrivacySandboxCurrentlyUnrestricted() const override;
  bool IsSubjectToM1NoticeRestricted() const override;
  bool IsRestrictedNoticeEnabled() const override;
  void OnCookiesCleared() override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  void SetDelegateForTesting(std::unique_ptr<Delegate> delegate) override;

  bool AreRelatedWebsiteSetsEnabled() const override;

 private:
  // Callback to ensure we don't ever enable the Privacy Sandbox.
  void OnPrivacySandboxPrefChanged();

  std::unique_ptr<Delegate> delegate_;
  raw_ptr<PrefService> pref_service_;
  PrefChangeRegistrar user_prefs_registrar_;
};

#endif  // BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_
