/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync/service/sync_session_durations_metrics_recorder.h"

namespace syncer {

SyncSessionDurationsMetricsRecorder::SyncSessionDurationsMetricsRecorder(
    SyncService* sync_service,
    signin::IdentityManager* identity_manager)
    : sync_service_(sync_service),
      identity_manager_(identity_manager),
      history_sync_recorder_(sync_service) {}

SyncSessionDurationsMetricsRecorder::~SyncSessionDurationsMetricsRecorder() {}

void SyncSessionDurationsMetricsRecorder::OnSessionStarted(
    base::TimeTicks session_start) {}
void SyncSessionDurationsMetricsRecorder::OnSessionEnded(
    base::TimeDelta session_length) {}

void SyncSessionDurationsMetricsRecorder::OnAccountsInCookieUpdated(
    const signin::AccountsInCookieJarInfo& accounts_in_cookie_jar_info,
    const GoogleServiceAuthError& error) {}
void SyncSessionDurationsMetricsRecorder::OnStateChanged(
    syncer::SyncService* sync) {}
void SyncSessionDurationsMetricsRecorder::OnPrimaryAccountChanged(
    const signin::PrimaryAccountChangeEvent& event) {}
void SyncSessionDurationsMetricsRecorder::OnRefreshTokenUpdatedForAccount(
    const CoreAccountInfo& account_info) {}
void SyncSessionDurationsMetricsRecorder::OnRefreshTokenRemovedForAccount(
    const CoreAccountId& account_id) {}
void SyncSessionDurationsMetricsRecorder::OnRefreshTokensLoaded() {}
void SyncSessionDurationsMetricsRecorder::
    OnErrorStateOfRefreshTokenUpdatedForAccount(
        const CoreAccountInfo& account_info,
        const GoogleServiceAuthError& error,
        signin_metrics::SourceForRefreshTokenOperation token_operation_source) {
}

SyncSessionDurationsMetricsRecorder::SigninStatus
SyncSessionDurationsMetricsRecorder::GetSigninStatus() const {
  return SyncSessionDurationsMetricsRecorder::SigninStatus::kSignedOut;
}

bool SyncSessionDurationsMetricsRecorder::IsSyncing() const {
  return false;
}
}  // namespace syncer
