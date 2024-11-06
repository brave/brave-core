/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/privacy_sandbox/brave_privacy_sandbox_settings.h"

#include <memory>
#include <string>
#include <utility>

#include "base/notreached.h"
#include "components/browsing_topics/common/common_types.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/privacy_sandbox/privacy_sandbox_prefs.h"
#include "components/privacy_sandbox/tracking_protection_settings.h"

BravePrivacySandboxSettings::BravePrivacySandboxSettings(
    std::unique_ptr<Delegate> delegate,
    HostContentSettingsMap* host_content_settings_map,
    content_settings::CookieSettings* cookie_settings,
    privacy_sandbox::TrackingProtectionSettings* tracking_protection_settings,
    PrefService* pref_service)
    : pref_service_(pref_service) {
  // Register observers for the Privacy Sandbox.
  user_prefs_registrar_.Init(pref_service_);
  user_prefs_registrar_.Add(
      prefs::kPrivacySandboxApisEnabled,
      base::BindRepeating(
          &BravePrivacySandboxSettings::OnPrivacySandboxPrefChanged,
          base::Unretained(this)));
  user_prefs_registrar_.Add(
      prefs::kPrivacySandboxApisEnabledV2,
      base::BindRepeating(
          &BravePrivacySandboxSettings::OnPrivacySandboxPrefChanged,
          base::Unretained(this)));
  user_prefs_registrar_.Add(
      prefs::kPrivacySandboxRelatedWebsiteSetsEnabled,
      base::BindRepeating(
          &BravePrivacySandboxSettings::OnPrivacySandboxPrefChanged,
          base::Unretained(this)));
}

BravePrivacySandboxSettings::~BravePrivacySandboxSettings() = default;

void BravePrivacySandboxSettings::OnPrivacySandboxPrefChanged() {
  // Make sure that Private Sandbox features remain disabled even if we manually
  // access the Pref service and try to change the preferences from there.
  if (pref_service_->GetBoolean(prefs::kPrivacySandboxApisEnabled)) {
    pref_service_->SetBoolean(prefs::kPrivacySandboxApisEnabled, false);
  }
  if (pref_service_->GetBoolean(prefs::kPrivacySandboxApisEnabledV2)) {
    pref_service_->SetBoolean(prefs::kPrivacySandboxApisEnabledV2, false);
  }
  if (pref_service_->GetBoolean(
          prefs::kPrivacySandboxRelatedWebsiteSetsEnabled)) {
    pref_service_->SetBoolean(prefs::kPrivacySandboxRelatedWebsiteSetsEnabled,
                              false);
  }
}

// PrivacySandboxSettings:

bool BravePrivacySandboxSettings::IsTopicsAllowed() const {
  return false;
}

bool BravePrivacySandboxSettings::IsTopicsAllowedForContext(
    const url::Origin& top_frame_origin,
    const GURL& url,
    content::RenderFrameHost* console_frame) const {
  return false;
}

bool BravePrivacySandboxSettings::IsTopicAllowed(
    const privacy_sandbox::CanonicalTopic& topic) {
  return false;
}

void BravePrivacySandboxSettings::SetTopicAllowed(
    const privacy_sandbox::CanonicalTopic& topic,
    bool allowed) {}

bool BravePrivacySandboxSettings::IsTopicPrioritized(
    const privacy_sandbox::CanonicalTopic& topic) {
  return false;
}

void BravePrivacySandboxSettings::ClearTopicSettings(base::Time start_time,
                                                     base::Time end_time) {}

base::Time BravePrivacySandboxSettings::TopicsDataAccessibleSince() const {
  // Future time means topics data is never accessible
  return base::Time::Max();
}

bool BravePrivacySandboxSettings::IsAttributionReportingEverAllowed() const {
  return false;
}

bool BravePrivacySandboxSettings::IsAttributionReportingAllowed(
    const url::Origin& top_frame_origin,
    const url::Origin& reporting_origin,
    content::RenderFrameHost* console_frame) const {
  return false;
}

bool BravePrivacySandboxSettings::MaySendAttributionReport(
    const url::Origin& source_origin,
    const url::Origin& destination_origin,
    const url::Origin& reporting_origin,
    content::RenderFrameHost* console_frame) const {
  return false;
}

bool BravePrivacySandboxSettings::
    IsAttributionReportingTransitionalDebuggingAllowed(
        const url::Origin& top_frame_origin,
        const url::Origin& reporting_origin,
        bool& can_bypass) const {
  return false;
}

void BravePrivacySandboxSettings::SetFledgeJoiningAllowed(
    const std::string& top_frame_etld_plus1,
    bool allowed) {}

void BravePrivacySandboxSettings::ClearFledgeJoiningAllowedSettings(
    base::Time start_time,
    base::Time end_time) {}

bool BravePrivacySandboxSettings::IsFledgeAllowed(
    const url::Origin& top_frame_origin,
    const url::Origin& auction_party,
    content::InterestGroupApiOperation interest_group_api_operation,
    content::RenderFrameHost* console_frame) const {
  return false;
}

bool BravePrivacySandboxSettings::IsEventReportingDestinationAttested(
    const url::Origin& destination_origin,
    privacy_sandbox::PrivacySandboxAttestationsGatedAPI invoking_api) const {
  return false;
}

bool BravePrivacySandboxSettings::IsSharedStorageAllowed(
    const url::Origin& top_frame_origin,
    const url::Origin& accessing_origin,
    std::string* out_debug_message,
    content::RenderFrameHost* console_frame,
    bool* out_block_is_site_setting_specific) const {
  return false;
}

bool BravePrivacySandboxSettings::IsSharedStorageSelectURLAllowed(
    const url::Origin& top_frame_origin,
    const url::Origin& accessing_origin,
    std::string* out_debug_message,
    bool* out_block_is_site_setting_specific) const {
  return false;
}

bool BravePrivacySandboxSettings::IsFencedStorageReadAllowed(
    const url::Origin& top_frame_origin,
    const url::Origin& accessing_origin,
    content::RenderFrameHost* console_frame) const {
  return false;
}

bool BravePrivacySandboxSettings::IsPrivateAggregationAllowed(
    const url::Origin& top_frame_origin,
    const url::Origin& reporting_origin,
    bool* out_block_is_site_setting_specific) const {
  return false;
}

bool BravePrivacySandboxSettings::IsPrivateAggregationDebugModeAllowed(
    const url::Origin& top_frame_origin,
    const url::Origin& reporting_origin) const {
  return false;
}

privacy_sandbox::TpcdExperimentEligibility
BravePrivacySandboxSettings::GetCookieDeprecationExperimentCurrentEligibility()
    const {
  return privacy_sandbox::TpcdExperimentEligibility(
      privacy_sandbox::TpcdExperimentEligibility::Reason::k3pCookiesBlocked);
}

bool BravePrivacySandboxSettings::IsCookieDeprecationLabelAllowed() const {
  return false;
}

bool BravePrivacySandboxSettings::IsCookieDeprecationLabelAllowedForContext(
    const url::Origin& top_frame_origin,
    const url::Origin& context_origin) const {
  return false;
}

void BravePrivacySandboxSettings::SetAllPrivacySandboxAllowedForTesting() {}
void BravePrivacySandboxSettings::SetTopicsBlockedForTesting() {}

bool BravePrivacySandboxSettings::IsPrivacySandboxRestricted() const {
  return true;
}

bool BravePrivacySandboxSettings::IsPrivacySandboxCurrentlyUnrestricted()
    const {
  return false;
}

bool BravePrivacySandboxSettings::IsSubjectToM1NoticeRestricted() const {
  return false;
}

bool BravePrivacySandboxSettings::IsRestrictedNoticeEnabled() const {
  return false;
}

void BravePrivacySandboxSettings::OnCookiesCleared() {}
void BravePrivacySandboxSettings::AddObserver(Observer* observer) {}
void BravePrivacySandboxSettings::RemoveObserver(Observer* observer) {}

void BravePrivacySandboxSettings::SetDelegateForTesting(
    std::unique_ptr<Delegate> delegate) {
  delegate_ = std::move(delegate);
}

bool BravePrivacySandboxSettings::AreRelatedWebsiteSetsEnabled() const {
  return false;
}
