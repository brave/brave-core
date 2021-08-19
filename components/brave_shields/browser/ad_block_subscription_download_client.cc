/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_subscription_download_client.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/metrics/histogram_macros_local.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_download_manager.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service_manager.h"
#include "components/download/public/background_service/download_metadata.h"
#include "services/network/public/cpp/resource_request_body.h"

namespace brave_shields {

AdBlockSubscriptionDownloadClient::AdBlockSubscriptionDownloadClient(
    AdBlockSubscriptionServiceManager* subscription_manager)
    : subscription_manager_(subscription_manager) {}

AdBlockSubscriptionDownloadClient::~AdBlockSubscriptionDownloadClient() =
    default;

AdBlockSubscriptionDownloadManager*
AdBlockSubscriptionDownloadClient::GetAdBlockSubscriptionDownloadManager() {
  return subscription_manager_->download_manager();
}

void AdBlockSubscriptionDownloadClient::OnServiceInitialized(
    bool state_lost,
    const std::vector<download::DownloadMetaData>& downloads) {
  AdBlockSubscriptionDownloadManager* download_manager =
      GetAdBlockSubscriptionDownloadManager();
  if (!download_manager)
    return;

  std::set<std::string> outstanding_download_guids;
  std::map<std::string, base::FilePath> successful_downloads;
  for (const auto& download : downloads) {
    if (!download.completion_info) {
      outstanding_download_guids.emplace(download.guid);
      continue;
    }

    successful_downloads.emplace(download.guid, download.completion_info->path);
  }

  download_manager->OnDownloadServiceReady(outstanding_download_guids,
                                           successful_downloads);
}

void AdBlockSubscriptionDownloadClient::OnServiceUnavailable() {
  AdBlockSubscriptionDownloadManager* download_manager =
      GetAdBlockSubscriptionDownloadManager();
  if (download_manager)
    download_manager->OnDownloadServiceUnavailable();
}

void AdBlockSubscriptionDownloadClient::OnDownloadFailed(
    const std::string& guid,
    const download::CompletionInfo& completion_info,
    download::Client::FailureReason reason) {
  AdBlockSubscriptionDownloadManager* download_manager =
      GetAdBlockSubscriptionDownloadManager();
  if (download_manager)
    download_manager->OnDownloadFailed(guid);
}

void AdBlockSubscriptionDownloadClient::OnDownloadSucceeded(
    const std::string& guid,
    const download::CompletionInfo& completion_info) {
  AdBlockSubscriptionDownloadManager* download_manager =
      GetAdBlockSubscriptionDownloadManager();
  if (!download_manager)
    return;

  std::string mimetype;
  if (!completion_info.response_headers->GetMimeType(&mimetype)) {
    download_manager->OnDownloadFailed(guid);
    return;
  }

  if (mimetype != "text/plain") {
    download_manager->OnDownloadFailed(guid);
    return;
  }

  download_manager->OnDownloadSucceeded(guid, completion_info.path);
}

bool AdBlockSubscriptionDownloadClient::CanServiceRemoveDownloadedFile(
    const std::string& guid,
    bool force_delete) {
  // Always return true. We immediately postprocess successful downloads and the
  // file downloaded by the Download Service should already be deleted and this
  // hypothetically should never be called with anything that matters.
  return true;
}

void AdBlockSubscriptionDownloadClient::GetUploadData(
    const std::string& guid,
    download::GetUploadDataCallback callback) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), nullptr));
}

}  // namespace brave_shields
