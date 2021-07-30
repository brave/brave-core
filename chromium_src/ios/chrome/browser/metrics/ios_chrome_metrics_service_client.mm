/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/metrics/ios_chrome_metrics_service_client.h"

#include <stdint.h>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "base/path_service.h"
#include "base/process/process_metrics.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "components/metrics/metrics_log_uploader.h"
#include "components/metrics/metrics_provider.h"
#include "components/metrics/metrics_service.h"
#include "components/metrics/metrics_service_client.h"
#include "components/metrics/metrics_state_manager.h"
#include "components/metrics/net/cellular_logic_helper.h"
#include "components/metrics/net/net_metrics_log_uploader.h"
#include "components/metrics/persistent_histograms.h"
#include "components/metrics/url_constants.h"
#include "components/metrics/version_utils.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/ukm/ukm_service.h"
#include "components/version_info/version_info.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/chrome_paths.h"
#include "ios/chrome/browser/google/google_brand.h"
#include "ios/chrome/browser/metrics/ios_chrome_stability_metrics_provider.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/web/public/thread/web_thread.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

void DeleteFileMetrics() {
  base::FilePath user_data_dir;
  if (base::PathService::Get(ios::DIR_USER_DATA, &user_data_dir)) {
    base::FilePath browser_metrics_upload_dir =
        user_data_dir.AppendASCII(kBrowserMetricsName);
    // When metrics reporting is not enabled, any existing files should be
    // deleted in order to preserve user privacy.
    base::ThreadPool::PostTask(
        FROM_HERE,
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
        base::BindOnce(base::GetDeletePathRecursivelyCallback(),
                       std::move(browser_metrics_upload_dir)));
  }
}

}  // namespace

IOSChromeMetricsServiceClient::IOSChromeMetricsServiceClient(
    metrics::MetricsStateManager* state_manager)
    : metrics_state_manager_(state_manager),
      stability_metrics_provider_(nullptr),
      weak_ptr_factory_(this) {
  DCHECK(thread_checker_.CalledOnValidThread());
  notification_listeners_active_ = RegisterForNotifications();
  metrics_state_manager_ = nullptr;
}

IOSChromeMetricsServiceClient::~IOSChromeMetricsServiceClient() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

// static
std::unique_ptr<IOSChromeMetricsServiceClient>
IOSChromeMetricsServiceClient::Create(
    metrics::MetricsStateManager* state_manager) {
  // Perform two-phase initialization so that |client->metrics_service_| only
  // receives pointers to fully constructed objects.
  std::unique_ptr<IOSChromeMetricsServiceClient> client(
      new IOSChromeMetricsServiceClient(state_manager));
  client->Initialize();

  return client;
}

// static
void IOSChromeMetricsServiceClient::RegisterPrefs(
    PrefRegistrySimple* registry) {
  metrics::MetricsService::RegisterPrefs(registry);
}

metrics::MetricsService* IOSChromeMetricsServiceClient::GetMetricsService() {
  return nullptr;
}

ukm::UkmService* IOSChromeMetricsServiceClient::GetUkmService() {
  return nullptr;
}

void IOSChromeMetricsServiceClient::SetMetricsClientId(
    const std::string& client_id) {}

int32_t IOSChromeMetricsServiceClient::GetProduct() {
  return metrics::ChromeUserMetricsExtension::CHROME;
}

std::string IOSChromeMetricsServiceClient::GetApplicationLocale() {
  return GetApplicationContext()->GetApplicationLocale();
}

bool IOSChromeMetricsServiceClient::GetBrand(std::string* brand_code) {
  return ios::google_brand::GetBrand(brand_code);
}

metrics::SystemProfileProto::Channel
IOSChromeMetricsServiceClient::GetChannel() {
  return metrics::AsProtobufChannel(::GetChannel());
}

bool IOSChromeMetricsServiceClient::IsExtendedStableChannel() {
  return false;  // Not supported on iOS.
}

std::string IOSChromeMetricsServiceClient::GetVersionString() {
  return metrics::GetVersionString();
}

void IOSChromeMetricsServiceClient::CollectFinalMetricsForLog(
    base::OnceClosure done_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  collect_final_metrics_done_callback_ = std::move(done_callback);
  CollectFinalHistograms();
}

std::unique_ptr<metrics::MetricsLogUploader>
IOSChromeMetricsServiceClient::CreateUploader(
    const GURL& server_url,
    const GURL& insecure_server_url,
    base::StringPiece mime_type,
    metrics::MetricsLogUploader::MetricServiceType service_type,
    const metrics::MetricsLogUploader::UploadCallback& on_upload_complete) {
  return std::make_unique<metrics::NetMetricsLogUploader>(
      GetApplicationContext()->GetSharedURLLoaderFactory(), server_url,
      insecure_server_url, mime_type, service_type, on_upload_complete);
}

base::TimeDelta IOSChromeMetricsServiceClient::GetStandardUploadInterval() {
  return metrics::GetUploadInterval(metrics::ShouldUseCellularUploadInterval());
}

void IOSChromeMetricsServiceClient::OnRendererProcessCrash() {
  stability_metrics_provider_->LogRendererCrash();
}

void IOSChromeMetricsServiceClient::WebStateDidStartLoading(
    web::WebState* web_state) {
  metrics_service_->OnApplicationNotIdle();
}

void IOSChromeMetricsServiceClient::WebStateDidStopLoading(
    web::WebState* web_state) {
  metrics_service_->OnApplicationNotIdle();
}

void IOSChromeMetricsServiceClient::Initialize() {
  RegisterMetricsServiceProviders();
}

void IOSChromeMetricsServiceClient::RegisterMetricsServiceProviders() {
  DeleteFileMetrics();
}

void IOSChromeMetricsServiceClient::RegisterUKMProviders() {}

void IOSChromeMetricsServiceClient::CollectFinalHistograms() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

bool IOSChromeMetricsServiceClient::RegisterForNotifications() {
  return false;
}

bool IOSChromeMetricsServiceClient::RegisterForBrowserStateEvents(
    ChromeBrowserState* browser_state) {
  return false;
}

void IOSChromeMetricsServiceClient::OnTabParented(web::WebState* web_state) {}

void IOSChromeMetricsServiceClient::OnURLOpenedFromOmnibox(OmniboxLog* log) {}

// static
metrics::FileMetricsProvider::FilterAction
IOSChromeMetricsServiceClient::FilterBrowserMetricsFiles(
    const base::FilePath& path) {
  // Do not process the file if it corresponds to the current process id.
  base::ProcessId pid;
  bool parse_success = base::GlobalHistogramAllocator::ParseFilePath(
      path, nullptr, nullptr, &pid);
  if (!parse_success)
    return metrics::FileMetricsProvider::FILTER_PROCESS_FILE;
  if (pid == base::GetCurrentProcId())
    return metrics::FileMetricsProvider::FILTER_ACTIVE_THIS_PID;
  // No need to test whether |pid| is a different active process. This isn't
  // applicable to iOS because there cannot be two copies of Chrome running.
  return metrics::FileMetricsProvider::FILTER_PROCESS_FILE;
}

metrics::EnableMetricsDefault
IOSChromeMetricsServiceClient::GetMetricsReportingDefaultState() {
  return metrics::GetMetricsReportingDefaultState(
      GetApplicationContext()->GetLocalState());
}

void IOSChromeMetricsServiceClient::OnHistoryDeleted() {
  if (ukm_service_)
    ukm_service_->Purge();
}

void IOSChromeMetricsServiceClient::OnUkmAllowedStateChanged(bool must_purge) {
  if (!ukm_service_)
    return;
  if (must_purge) {
    ukm_service_->Purge();
    ukm_service_->ResetClientState(ukm::ResetReason::kOnUkmAllowedStateChanged);
  }
  // Signal service manager to enable/disable UKM based on new state.
  UpdateRunningServices();
}

void IOSChromeMetricsServiceClient::OnIncognitoWebStateAdded() {
  // Signal service manager to enable/disable UKM based on new state.
  UpdateRunningServices();
}

void IOSChromeMetricsServiceClient::OnIncognitoWebStateRemoved() {
  // Signal service manager to enable/disable UKM based on new state.
  UpdateRunningServices();
}

bool IOSChromeMetricsServiceClient::IsUkmAllowedForAllProfiles() {
  return false;
}

bool IOSChromeMetricsServiceClient::
    AreNotificationListenersEnabledOnAllProfiles() {
  return false;
}

std::string IOSChromeMetricsServiceClient::GetUploadSigningKey() {
  return "";
}
