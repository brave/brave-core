/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/metrics/metrics_service.h"

#include <memory>
#include <string_view>

#include "base/callback_list.h"
#include "base/metrics/histogram_base.h"
#include "base/time/time.h"
#include "components/metrics/clean_exit_beacon.h"
#include "components/metrics/metrics_log.h"
#include "components/metrics/metrics_logs_event_manager.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_rotation_scheduler.h"
#include "components/metrics/metrics_service_client.h"
#include "components/metrics/metrics_service_observer.h"
#include "components/metrics/metrics_state_manager.h"
#include "components/metrics/stability_metrics_provider.h"
#include "components/prefs/pref_service.h"
#include "components/variations/synthetic_trial_registry.h"

namespace metrics {

// static
void MetricsService::RegisterPrefs(PrefRegistrySimple* registry) {
  CleanExitBeacon::RegisterPrefs(registry);
  MetricsStateManager::RegisterPrefs(registry);
  MetricsLog::RegisterPrefs(registry);
  StabilityMetricsProvider::RegisterPrefs(registry);
  MetricsReportingService::RegisterPrefs(registry);

  registry->RegisterIntegerPref(prefs::kMetricsSessionID, -1);
}

MetricsService::MetricsService(MetricsStateManager* state_manager,
                               MetricsServiceClient* client,
                               PrefService* local_state)
    : reporting_service_(client, local_state, &logs_event_manager_),
      state_manager_(state_manager),
      client_(client),
      local_state_(local_state),
      recording_state_(UNSET),
      test_mode_active_(false),
      state_(CONSTRUCTED),
      idle_since_last_transmission_(false),
      session_id_(-1) {}

MetricsService::~MetricsService() {}

void MetricsService::InitializeMetricsRecordingState() {}

void MetricsService::Start() {}

void MetricsService::StartRecordingForTests() {}

void MetricsService::StartUpdatingLastLiveTimestamp() {}

void MetricsService::Stop() {}

void MetricsService::EnableReporting() {}

void MetricsService::DisableReporting() {}

std::string MetricsService::GetClientId() const {
  return std::string();
}

int MetricsService::GetLowEntropySource() {
  return 0;
}

int MetricsService::GetOldLowEntropySource() {
  return 0;
}

int MetricsService::GetPseudoLowEntropySource() {
  return 0;
}

std::string_view MetricsService::GetLimitedEntropyRandomizationSource() {
  return std::string_view();
}

void MetricsService::SetExternalClientId(const std::string& id) {}

bool MetricsService::WasLastShutdownClean() const {
  return true;
}

void MetricsService::EnableRecording() {}

void MetricsService::DisableRecording() {}

bool MetricsService::recording_active() const {
  return false;
}

bool MetricsService::reporting_active() const {
  return false;
}

bool MetricsService::has_unsent_logs() const {
  return false;
}

bool MetricsService::IsMetricsReportingEnabled() const {
  return false;
}

void MetricsService::HandleIdleSinceLastTransmission(bool in_idle) {}

void MetricsService::OnApplicationNotIdle() {}

#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
void MetricsService::OnAppEnterBackground(bool keep_recording_in_background) {}

void MetricsService::OnAppEnterForeground(bool force_open_new_log) {}
#endif  // BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)

void MetricsService::OnPageLoadStarted() {}

void MetricsService::LogCleanShutdown() {}

void MetricsService::ClearSavedStabilityMetrics() {}

void MetricsService::MarkCurrentHistogramsAsReported() {}

variations::SyntheticTrialRegistry*
MetricsService::GetSyntheticTrialRegistry() {
  return client_->GetSyntheticTrialRegistry();
}

base::TimeDelta MetricsService::GetInitializationDelay() {
  return base::TimeDelta();
}

base::TimeDelta MetricsService::GetUpdateLastAliveTimestampDelay() {
  return base::TimeDelta();
}

bool MetricsService::StageCurrentLogForTest() {
  return false;
}

//------------------------------------------------------------------------------
// private methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Initialization methods

void MetricsService::InitializeMetricsState() {}

void MetricsService::OnUserAction(const std::string& action,
                                  base::TimeTicks action_time) {}

void MetricsService::FinishedInitTask() {}

void MetricsService::GetUptimes(PrefService* pref,
                                base::TimeDelta* incremental_uptime,
                                base::TimeDelta* uptime) {}

//------------------------------------------------------------------------------
// Recording control methods

void MetricsService::OpenNewLog(bool call_providers) {}

MetricsService::FinalizedLog::FinalizedLog() = default;
MetricsService::FinalizedLog::~FinalizedLog() = default;
MetricsService::FinalizedLog::FinalizedLog(FinalizedLog&& other) = default;
MetricsService::FinalizedLog& MetricsService::FinalizedLog::operator=(
    FinalizedLog&& other) = default;

MetricsService::MetricsLogHistogramWriter::MetricsLogHistogramWriter(
    MetricsLog* log)
    : MetricsLogHistogramWriter(log,
                                base::Histogram::kUmaTargetedHistogramFlag) {}

MetricsService::MetricsLogHistogramWriter::MetricsLogHistogramWriter(
    MetricsLog* log,
    base::HistogramBase::Flags required_flags)
    : required_flags_(required_flags),
      flattener_(nullptr),
      histogram_snapshot_manager_(nullptr),
      snapshot_transaction_id_(0) {}

MetricsService::MetricsLogHistogramWriter::~MetricsLogHistogramWriter() =
    default;

void MetricsService::MetricsLogHistogramWriter::
    SnapshotStatisticsRecorderDeltas() {}

void MetricsService::MetricsLogHistogramWriter::
    SnapshotStatisticsRecorderUnloggedSamples() {}

MetricsService::IndependentMetricsLoader::IndependentMetricsLoader(
    std::unique_ptr<MetricsLog> log,
    std::string app_version,
    std::string signing_key)
    : log_(std::move(log)),
      flattener_(nullptr),
      snapshot_manager_(nullptr),
      app_version_(std::move(app_version)),
      signing_key_(std::move(signing_key)) {}

MetricsService::IndependentMetricsLoader::~IndependentMetricsLoader() = default;

void MetricsService::IndependentMetricsLoader::Run(
    base::OnceCallback<void(bool)> done_callback,
    MetricsProvider* metrics_provider) {}

void MetricsService::IndependentMetricsLoader::FinalizeLog() {}

bool MetricsService::IndependentMetricsLoader::HasFinalizedLog() {
  return false;
}

MetricsService::FinalizedLog
MetricsService::IndependentMetricsLoader::ReleaseFinalizedLog() {
  return FinalizedLog();
}

void MetricsService::StartInitTask() {}

void MetricsService::CloseCurrentLog(
    bool async,
    MetricsLogsEventManager::CreateReason reason,
    base::OnceClosure log_stored_callback) {}

void MetricsService::StoreFinalizedLog(
    MetricsLog::LogType log_type,
    MetricsLogsEventManager::CreateReason reason,
    base::OnceClosure done_callback,
    FinalizedLog finalized_log) {}

void MetricsService::MaybeCleanUpAndStoreFinalizedLog(
    std::unique_ptr<MetricsLogHistogramWriter> log_histogram_writer,
    MetricsLog::LogType log_type,
    MetricsLogsEventManager::CreateReason reason,
    base::OnceClosure done_callback,
    FinalizedLog finalized_log) {}

void MetricsService::PushPendingLogsToPersistentStorage(
    MetricsLogsEventManager::CreateReason reason) {}

//------------------------------------------------------------------------------
// Transmission of logs methods

void MetricsService::StartSchedulerIfNecessary() {}

void MetricsService::StartScheduledUpload() {}

void MetricsService::OnFinalLogInfoCollectionDone() {}

void MetricsService::OnAsyncPeriodicOngoingLogStored() {}

bool MetricsService::PrepareInitialStabilityLog(
    const std::string& prefs_previous_version) {
  return false;
}

void MetricsService::RegisterMetricsProvider(
    std::unique_ptr<MetricsProvider> provider) {
  DCHECK_EQ(CONSTRUCTED, state_);
  delegating_provider_.RegisterMetricsProvider(std::move(provider));
}

void MetricsService::CheckForClonedInstall() {}

bool MetricsService::ShouldResetClientIdsOnClonedInstall() {
  return false;
}

std::unique_ptr<MetricsLog> MetricsService::CreateLog(
    MetricsLog::LogType log_type) {
  return nullptr;
}

void MetricsService::AddLogsObserver(
    MetricsLogsEventManager::Observer* observer) {}

void MetricsService::RemoveLogsObserver(
    MetricsLogsEventManager::Observer* observer) {}

base::CallbackListSubscription MetricsService::AddEnablementObserver(
    const base::RepeatingCallback<void(bool)>& observer) {
  return base::CallbackListSubscription();
}

void MetricsService::SetPersistentSystemProfile(
    const std::string& serialized_proto,
    bool complete) {}

// static
std::string MetricsService::RecordCurrentEnvironmentHelper(
    MetricsLog* log,
    PrefService* local_state,
    DelegatingProvider* delegating_provider) {
  return std::string();
}

void MetricsService::RecordCurrentEnvironment(MetricsLog* log, bool complete) {}

void MetricsService::PrepareProviderMetricsLogDone(
    std::unique_ptr<IndependentMetricsLoader> loader,
    bool success) {}

bool MetricsService::PrepareProviderMetricsLog() {
  return false;
}

void MetricsService::PrepareProviderMetricsTask() {}

void MetricsService::UpdateLastLiveTimestampTask() {}

bool MetricsService::IsTooEarlyToCloseLog() {
  return false;
}

void MetricsService::OnClonedInstallDetected() {}

// static
MetricsService::FinalizedLog MetricsService::SnapshotDeltasAndFinalizeLog(
    std::unique_ptr<MetricsLogHistogramWriter> log_histogram_writer,
    std::unique_ptr<MetricsLog> log,
    bool truncate_events,
    std::optional<ChromeUserMetricsExtension::RealLocalTime> close_time,
    std::string&& current_app_version,
    std::string&& signing_key) {
  return FinalizedLog();
}

// static
MetricsService::FinalizedLog
MetricsService::SnapshotUnloggedSamplesAndFinalizeLog(
    MetricsLogHistogramWriter* log_histogram_writer,
    std::unique_ptr<MetricsLog> log,
    bool truncate_events,
    std::optional<ChromeUserMetricsExtension::RealLocalTime> close_time,
    std::string&& current_app_version,
    std::string&& signing_key) {
  return FinalizedLog();
}

// static
MetricsService::FinalizedLog MetricsService::FinalizeLog(
    std::unique_ptr<MetricsLog> log,
    bool truncate_events,
    std::optional<ChromeUserMetricsExtension::RealLocalTime> close_time,
    const std::string& current_app_version,
    const std::string& signing_key) {
  return FinalizedLog();
}

}  // namespace metrics
