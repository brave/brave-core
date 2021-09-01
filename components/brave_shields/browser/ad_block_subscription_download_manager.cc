/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_subscription_download_manager.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/metrics/histogram_functions.h"
#include "base/task/post_task.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "build/build_config.h"
#include "components/download/public/background_service/background_download_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace brave_shields {

namespace {

const net::NetworkTrafficAnnotationTag
    kBraveShieldsAdBlockSubscriptionTrafficAnnotation =
        net::DefineNetworkTrafficAnnotation(
            "brave_shields_ad_block_subscription",
            R"(
        semantics {
          sender: "Brave Shields"
          description:
            "Brave periodically downloads updates to third-party filter lists "
            "added by users on brave://adblock."
          trigger:
            "After being registered in brave://adblock, any enabled filter "
            "list subscriptions will be updated in accordance with their "
            "`Expires` field if present, or daily otherwise. A manual refresh "
            "for a particular list can also be triggered in brave://adblock."
          data: "The URL endpoint provided by the user in brave://adblock to "
            "fetch list updates from. No user information is sent."
          destination: BRAVE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This request cannot be disabled in settings. However it will "
            "never be made if the corresponding entry is removed from the "
            "brave://adblock page's custom list subscription section."
          policy_exception_justification: "Not yet implemented."
        })");

}  // namespace

AdBlockSubscriptionDownloadManager::AdBlockSubscriptionDownloadManager(
    download::BackgroundDownloadService* download_service,
    scoped_refptr<base::SequencedTaskRunner> background_task_runner)
    : download_service_(download_service),
      is_available_for_downloads_(true),
      background_task_runner_(background_task_runner) {}

AdBlockSubscriptionDownloadManager::~AdBlockSubscriptionDownloadManager() =
    default;

void AdBlockSubscriptionDownloadManager::StartDownload(const GURL& download_url,
                                                       bool from_ui) {
  download::DownloadParams download_params;
  download_params.client = download::DownloadClient::CUSTOM_LIST_SUBSCRIPTIONS;
  download_params.guid = base::GenerateGUID();
  download_params.callback = base::BindRepeating(
      &AdBlockSubscriptionDownloadManager::OnDownloadStarted, AsWeakPtr(),
      download_url);
  download_params.traffic_annotation = net::MutableNetworkTrafficAnnotationTag(
      kBraveShieldsAdBlockSubscriptionTrafficAnnotation);
  download_params.request_params.url = download_url;
  download_params.request_params.method = "GET";
  if (from_ui) {
    // This triggers a high priority download with no network restrictions to
    // provide status feedback as quickly as possible.
    download_params.scheduling_params.priority =
        download::SchedulingParams::Priority::UI;
    download_params.scheduling_params.battery_requirements =
        download::SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE;
    download_params.scheduling_params.network_requirements =
        download::SchedulingParams::NetworkRequirements::NONE;
  } else {
    download_params.scheduling_params.priority =
        download::SchedulingParams::Priority::NORMAL;
    download_params.scheduling_params.battery_requirements =
        download::SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE;
    download_params.scheduling_params.network_requirements =
        download::SchedulingParams::NetworkRequirements::OPTIMISTIC;
  }

  download_service_->StartDownload(std::move(download_params));
}

void AdBlockSubscriptionDownloadManager::CancelAllPendingDownloads() {
  for (const std::pair<std::string, GURL> pending_download :
       pending_download_guids_) {
    const std::string& pending_download_guid = pending_download.first;
    download_service_->CancelDownload(pending_download_guid);
  }
}

bool AdBlockSubscriptionDownloadManager::IsAvailableForDownloads() const {
  return is_available_for_downloads_;
}

void AdBlockSubscriptionDownloadManager::Shutdown() {
  is_available_for_downloads_ = false;
  CancelAllPendingDownloads();
  // notify
}

void AdBlockSubscriptionDownloadManager::OnDownloadServiceReady(
    const std::set<std::string>& pending_download_guids,
    const std::map<std::string, base::FilePath>& successful_downloads) {
  // Ignore any pending guids because they will just retry automatically
  // and we we don't have the url to map them to
}

void AdBlockSubscriptionDownloadManager::OnDownloadServiceUnavailable() {
  is_available_for_downloads_ = false;
}

void AdBlockSubscriptionDownloadManager::OnDownloadStarted(
    const GURL download_url,
    const std::string& guid,
    download::DownloadParams::StartResult start_result) {
  if (start_result == download::DownloadParams::StartResult::ACCEPTED) {
    pending_download_guids_.insert(
        std::pair<std::string, GURL>(guid, download_url));
  }
}

void AdBlockSubscriptionDownloadManager::OnDownloadFailed(
    const std::string& guid) {
  auto it = pending_download_guids_.find(guid);
  if (it == pending_download_guids_.end()) {
    return;
  }
  GURL download_url = it->second;
  pending_download_guids_.erase(guid);

  base::UmaHistogramBoolean(
      "BraveShields.AdBlockSubscriptionDownloadManager.DownloadSucceeded",
      false);

  on_download_failed_callback_.Run(download_url);
}

bool EnsureDirExists(const base::FilePath& destination_dir) {
  return base::CreateDirectory(destination_dir);
}

void AdBlockSubscriptionDownloadManager::OnDownloadSucceeded(
    const std::string& guid,
    base::FilePath downloaded_file) {
  auto it = pending_download_guids_.find(guid);
  DCHECK(it != pending_download_guids_.end());
  GURL download_url = it->second;
  pending_download_guids_.erase(guid);

  base::UmaHistogramBoolean(
      "BraveShields.AdBlockSubscriptionDownloadManager.DownloadSucceeded",
      true);

  background_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&EnsureDirExists,
                     subscription_path_callback_.Run(download_url)),
      base::BindOnce(&AdBlockSubscriptionDownloadManager::OnDirCreated,
                     AsWeakPtr(), downloaded_file, download_url));
}

void AdBlockSubscriptionDownloadManager::OnDirCreated(
    base::FilePath downloaded_file,
    const GURL& download_url,
    bool created) {
  if (!created) {
    on_download_failed_callback_.Run(download_url);
    return;
  }

  base::FilePath list_path = subscription_path_callback_.Run(download_url)
                                 .Append(kCustomSubscriptionListText);

  background_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::ReplaceFile, downloaded_file, list_path, nullptr),
      base::BindOnce(&AdBlockSubscriptionDownloadManager::ReplaceFileCallback,
                     AsWeakPtr(), download_url));
}

void AdBlockSubscriptionDownloadManager::ReplaceFileCallback(
    const GURL& download_url,
    bool success) {
  if (!success) {
    on_download_failed_callback_.Run(download_url);
    return;
  }

  // this should send the data to subscription manager
  on_download_succeeded_callback_.Run(download_url);
}

}  // namespace brave_shields
