/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_uma_util.h"

// Make all public PermissionUmaUtil methods no-ops since we don't record
// permissions UKM. Methods below are in the same order they appear in the
// header.

namespace permissions {

using blink::PermissionType;

// These are needed by tests:
const char PermissionUmaUtil::kPermissionsPromptShown[] =
    "Permissions.Prompt.Shown";
const char PermissionUmaUtil::kPermissionsPromptShownGesture[] =
    "Permissions.Prompt.Shown.Gesture";
const char PermissionUmaUtil::kPermissionsPromptShownNoGesture[] =
    "Permissions.Prompt.Shown.NoGesture";
const char PermissionUmaUtil::kPermissionsPromptAccepted[] =
    "Permissions.Prompt.Accepted";
const char PermissionUmaUtil::kPermissionsPromptAcceptedGesture[] =
    "Permissions.Prompt.Accepted.Gesture";
const char PermissionUmaUtil::kPermissionsPromptAcceptedNoGesture[] =
    "Permissions.Prompt.Accepted.NoGesture";
const char PermissionUmaUtil::kPermissionsPromptAcceptedOnce[] =
    "Permissions.Prompt.AcceptedOnce";
const char PermissionUmaUtil::kPermissionsPromptAcceptedOnceGesture[] =
    "Permissions.Prompt.AcceptedOnce.Gesture";
const char PermissionUmaUtil::kPermissionsPromptAcceptedOnceNoGesture[] =
    "Permissions.Prompt.AcceptedOnce.NoGesture";
const char PermissionUmaUtil::kPermissionsPromptDenied[] =
    "Permissions.Prompt.Denied";
const char PermissionUmaUtil::kPermissionsPromptDeniedGesture[] =
    "Permissions.Prompt.Denied.Gesture";
const char PermissionUmaUtil::kPermissionsPromptDeniedNoGesture[] =
    "Permissions.Prompt.Denied.NoGesture";
const char PermissionUmaUtil::kPermissionsPromptDismissed[] =
    "Permissions.Prompt.Dismissed";
const char PermissionUmaUtil::kPermissionsExperimentalUsagePrefix[] =
    "Permissions.Experimental.Usage.";
const char PermissionUmaUtil::kPermissionsActionPrefix[] =
    "Permissions.Action.";

void PermissionUmaUtil::PermissionRequested(ContentSettingsType content_type) {}

// static
void PermissionUmaUtil::RecordActivityIndicator(
    std::set<ContentSettingsType> permissions,
    bool blocked,
    bool blocked_system_level,
    bool clicked) {}

// static
void PermissionUmaUtil::RecordDismissalType(
    const std::vector<ContentSettingsType>& content_settings_types,
    PermissionPromptDisposition ui_disposition,
    DismissalType dismissalType) {}

// static
void PermissionUmaUtil::RecordPermissionRequestedFromFrame(
    ContentSettingsType content_settings_type,
    content::RenderFrameHost* rfh) {}

// static
void PermissionUmaUtil::PermissionRequestPreignored(PermissionType permission) {
}

// static
void PermissionUmaUtil::PermissionRevoked(
    ContentSettingsType permission,
    PermissionSourceUI source_ui,
    const GURL& revoked_origin,
    content::BrowserContext* browser_context) {}

// static
void PermissionUmaUtil::RecordEmbargoPromptSuppression(
    PermissionEmbargoStatus embargo_status) {}

// static
void PermissionUmaUtil::RecordEmbargoPromptSuppressionFromSource(
    content::PermissionStatusSource source) {}

// static
void PermissionUmaUtil::RecordEmbargoStatus(
    PermissionEmbargoStatus embargo_status) {}

// static
void PermissionUmaUtil::RecordPermissionRecoverySuccessRate(
    ContentSettingsType permission,
    bool is_used,
    bool show_infobar,
    bool page_reload) {}

// static
void PermissionUmaUtil::RecordPermissionPromptAttempt(
    const std::vector<std::unique_ptr<PermissionRequest>>& requests,
    bool can_display_prompt) {}

// static
void PermissionUmaUtil::PermissionPromptShown(
    const std::vector<std::unique_ptr<PermissionRequest>>& requests) {}

// static
void PermissionUmaUtil::PermissionPromptResolved(
    const std::vector<std::unique_ptr<PermissionRequest>>& requests,
    content::BrowserContext* browser_context,
    PermissionAction permission_action,
    base::TimeDelta time_to_action,
    PermissionPromptDisposition ui_disposition,
    std::optional<PermissionPromptDispositionReason> ui_reason,
    std::optional<std::vector<ElementAnchoredBubbleVariant>> variants,
    std::optional<PermissionUiSelector::PredictionGrantLikelihood>
        predicted_grant_likelihood,
    std::optional<PermissionRequestRelevance> permission_request_relevance,
    std::optional<permissions::PermissionAiRelevanceModel>
        permission_ai_relevance_model,
    std::optional<bool> prediction_decision_held_back,
    std::optional<permissions::PermissionIgnoredReason> ignored_reason,
    bool did_show_prompt,
    bool did_click_managed,
    bool did_click_learn_more,
    std::optional<GeolocationAccuracy> initial_geolocation_accuracy_selection) {
}

// static
void PermissionUmaUtil::RecordCrowdDenyDelayedPushNotification(
    base::TimeDelta delay) {}

// static
void PermissionUmaUtil::RecordCrowdDenyVersionAtAbuseCheckTime(
    const std::optional<base::Version>& version) {}

// static
void PermissionUmaUtil::RecordElementAnchoredBubbleDismiss(
    const std::vector<std::unique_ptr<PermissionRequest>>& requests,
    DismissedReason reason) {}

// static
void PermissionUmaUtil::RecordElementAnchoredBubbleOsMetrics(
    const std::vector<std::unique_ptr<PermissionRequest>>& requests,
    OsScreen screen,
    OsScreenAction action,
    base::TimeDelta time_to_action) {}

// static
void PermissionUmaUtil::RecordElementAnchoredBubbleVariantUMA(
    const std::vector<std::unique_ptr<PermissionRequest>>& requests,
    ElementAnchoredBubbleVariant variant) {}

// static
void PermissionUmaUtil::RecordMissingPermissionInfobarShouldShow(
    bool should_show,
    const std::vector<ContentSettingsType>& content_settings_types) {}

// static
void PermissionUmaUtil::RecordMissingPermissionInfobarAction(
    PermissionAction action,
    const std::vector<ContentSettingsType>& content_settings_types) {}

// static
void PermissionUmaUtil::RecordPermissionUsage(
    ContentSettingsType permission_type,
    content::BrowserContext* browser_context,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin) {}

// static
void PermissionUmaUtil::RecordPermissionUsageNotificationShown(
    bool did_user_always_allow_notifications,
    bool is_allowlisted,
    int suspicious_score,
    content::BrowserContext* browser_context,
    const GURL& requesting_origin,
    uint64_t site_engagement_level) {}

// static
void PermissionUmaUtil::RecordTimeElapsedBetweenGrantAndUse(
    ContentSettingsType type,
    base::TimeDelta delta,
    content_settings::SettingSource source) {}

// static
void PermissionUmaUtil::RecordTimeElapsedBetweenGrantAndRevoke(
    ContentSettingsType type,
    base::TimeDelta delta) {}

// static
void PermissionUmaUtil::RecordAutoDSEPermissionReverted(
    ContentSettingsType permission_type,
    ContentSetting backed_up_setting,
    ContentSetting effective_setting,
    ContentSetting end_state_setting) {}

// static
void PermissionUmaUtil::RecordDSEEffectiveSetting(
    ContentSettingsType permission_type,
    ContentSetting setting) {}

// static
void PermissionUmaUtil::RecordPermissionPredictionConcurrentRequests(
    RequestType request_type) {}

// static
void PermissionUmaUtil::RecordPermissionPredictionSource(
    PermissionPredictionSource prediction_source,
    RequestType request_type) {}

// static
void PermissionUmaUtil::RecordPermissionPredictionServiceHoldback(
    RequestType request_type,
    PredictionModelType model_type,
    bool is_heldback) {}

// static
std::string PermissionUmaUtil::GetOneTimePermissionEventHistogram(
    ContentSettingsType type) {
  return "";
}

// static
void PermissionUmaUtil::RecordOneTimePermissionEvent(
    ContentSettingsType type,
    OneTimePermissionEvent event) {}

// static
void PermissionUmaUtil::RecordPageInfoPermissionChangeWithin1m(
    ContentSettingsType type,
    PermissionAction previous_action,
    ContentSetting setting_after) {}

// static
void PermissionUmaUtil::RecordPageInfoPermissionChange(
    ContentSettingsType type,
    ContentSetting setting_before,
    ContentSetting setting_after,
    bool suppress_reload_page_bar) {}

// static
std::string PermissionUmaUtil::GetPermissionActionString(
    PermissionAction permission_action) {
  return "";
}

// static
std::string PermissionUmaUtil::GetPredictionModelString(
    PredictionModelType model_type) {
  return "";
}

// static
std::string PermissionUmaUtil::GetPromptDispositionString(
    PermissionPromptDisposition ui_disposition) {
  return "";
}

// static
std::string PermissionUmaUtil::GetPromptDispositionReasonString(
    PermissionPromptDispositionReason ui_disposition_reason) {
  return "";
}

// static
std::string PermissionUmaUtil::GetRequestTypeString(RequestType request_type) {
  return "";
}

// static
bool PermissionUmaUtil::IsPromptDispositionQuiet(
    PermissionPromptDisposition prompt_disposition) {
  return false;
}

// static
bool PermissionUmaUtil::IsPromptDispositionLoud(
    PermissionPromptDisposition prompt_disposition) {
  return false;
}

// static
void PermissionUmaUtil::RecordIgnoreReason(
    const std::vector<std::unique_ptr<PermissionRequest>>& requests,
    PermissionPromptDisposition prompt_disposition,
    PermissionIgnoredReason reason) {}

// static
void PermissionUmaUtil::RecordPermissionsUsageSourceAndPolicyConfiguration(
    ContentSettingsType content_settings_type,
    content::RenderFrameHost* render_frame_host) {}

// static
void PermissionUmaUtil::RecordCrossOriginFrameActionAndPolicyConfiguration(
    ContentSettingsType content_settings_type,
    PermissionAction action,
    content::RenderFrameHost* render_frame_host) {}

// static
void PermissionUmaUtil::RecordTopLevelPermissionsHeaderPolicyOnNavigation(
    content::RenderFrameHost* render_frame_host) {}

// static
void PermissionUmaUtil::RecordPermissionRegrantForUnusedSites(
    const GURL& origin,
    ContentSettingsType content_settings_type,
    PermissionSourceUI source_ui,
    content::BrowserContext* browser_context,
    base::Time current_time) {}

// static
std::optional<uint32_t>
PermissionUmaUtil::GetDaysSinceUnusedSitePermissionRevocation(
    const GURL& origin,
    ContentSettingsType content_settings_type,
    base::Time current_time,
    HostContentSettingsMap* hcsm) {
  return std::nullopt;
}

// static
void PermissionUmaUtil::RecordPageReloadInfoBarShown(bool shown) {}

// static
void PermissionUmaUtil::RecordElementAnchoredPermissionPromptAction(
    const std::vector<std::unique_ptr<PermissionRequest>>& requests,
    const std::vector<base::WeakPtr<permissions::PermissionRequest>>&
        screen_requests,
    ElementAnchoredBubbleAction action,
    ElementAnchoredBubbleVariant variant,
    int screen_counter,
    const GURL& requesting_origin,
    content::BrowserContext* browser_context) {}

// static
void PermissionUmaUtil::RecordPermissionIndicatorElapsedTimeSinceLastUsage(
    RequestTypeForUma request_type,
    base::TimeDelta time_delta) {}

// static
void PermissionUmaUtil::RecordPermissionRequestRelevance(
    permissions::RequestType permission_request_type,
    PermissionRequestRelevance permission_request_relevance,
    PredictionModelType model_type) {}

// static
void PermissionUmaUtil::RecordBrowserAlwaysActiveWhilePrompting(
    RequestTypeForUma request_type,
    bool embedded_permission_element_initiated,
    bool always_active) {}

// static
void PermissionUmaUtil::RecordActionBrowserAlwaysActive(
    RequestTypeForUma request_type,
    std::string permission_action,
    bool always_active) {}

// static
void PermissionUmaUtil::RecordPredictionModelInquireTime(
    PredictionModelType model_type,
    base::TimeTicks model_inquire_start_time) {}

// static
void PermissionUmaUtil::RecordSnapshotTakenTimeAndSuccessForAivX(
    PredictionModelType model_type,
    base::TimeTicks snapshot_inquire_start_time,
    bool success) {}

// static
void PermissionUmaUtil::RecordRenderedTextAcquireSuccessForAivX(
    PredictionModelType model_type,
    bool success) {}

// static
void PermissionUmaUtil::RecordRenderedTextSize(PredictionModelType model_type,
                                               RequestType request_type,
                                               size_t text_size) {}

// static
void PermissionUmaUtil::RecordTryCancelPreviousEmbeddingsModelExecution(
    PredictionModelType model_type,
    bool cancel_previous_task) {}

// static
void PermissionUmaUtil::RecordFinishedPassageEmbeddingsTaskOutdated(
    PredictionModelType model_type,
    bool outdated) {}

// static
void PermissionUmaUtil::RecordPassageEmbeddingModelExecutionTimeAndStatus(
    PredictionModelType model_type,
    base::TimeTicks model_inquire_start_time,
    passage_embeddings::ComputeEmbeddingsStatus status) {}

// static
void PermissionUmaUtil::RecordLanguageDetectionStatus(
    LanguageDetectionStatus status) {}

// static
void PermissionUmaUtil::RecordPassageEmbeddingsCalculationTimeout(
    bool timeout) {}

// static
void PermissionUmaUtil::RecordPassageEmbedderMetadataValid(bool valid) {}

// static
void PermissionUmaUtil::RecordPredictionServiceTimeout(bool timeout) {}

// static
void PermissionUmaUtil::RecordPromptShownInActiveBrowser(
    RequestTypeForUma request_type,
    bool embedded_permission_element_initiated,
    bool active) {}

// static
void PermissionUmaUtil::RecordPermissionAutoRejectForActor(
    ContentSettingsType permission,
    bool is_actor_operating) {}

// static
void PermissionUmaUtil::RecordPrePromptSessionDuration(
    ContentSettingsType permission,
    base::TimeTicks request_first_display_time) {}

// static
void PermissionUmaUtil::RecordPostPromptSessionDuration(
    ContentSettingsType permission,
    base::TimeTicks request_first_display_time) {}

PermissionUmaUtil::ScopedRevocationReporter::ScopedRevocationReporter(
    content::BrowserContext* browser_context,
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    PermissionSourceUI source_ui) {
  content_type_ = ContentSettingsType::DEFAULT;
  source_ui_ = PermissionSourceUI::UNIDENTIFIED;
  is_initially_allowed_ = false;
}

PermissionUmaUtil::ScopedRevocationReporter::ScopedRevocationReporter(
    content::BrowserContext* browser_context,
    const GURL& primary_url,
    const GURL& secondary_url,
    ContentSettingsType content_type,
    PermissionSourceUI source_ui) {
  content_type_ = ContentSettingsType::DEFAULT;
  source_ui_ = PermissionSourceUI::UNIDENTIFIED;
  is_initially_allowed_ = false;
}

PermissionUmaUtil::ScopedRevocationReporter::~ScopedRevocationReporter() {}

// static
bool PermissionUmaUtil::ScopedRevocationReporter::IsInstanceInScope() {
  return false;
}

}  // namespace permissions
