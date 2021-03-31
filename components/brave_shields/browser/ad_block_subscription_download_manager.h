/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_DOWNLOAD_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_DOWNLOAD_MANAGER_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/memory/weak_ptr.h"
#include "components/download/public/background_service/download_params.h"

#include "components/services/storage/public/mojom/blob_storage_context.mojom.h"

namespace storage {
class BlobDataHandle;
}

namespace download {
class DownloadService;
}  // namespace download

namespace brave_shields {

class AdBlockSubscriptionDownloadClient;

// Manages the downloads of filter lists for custom subscriptions.
class AdBlockSubscriptionDownloadManager {
 public:
  AdBlockSubscriptionDownloadManager(
      download::DownloadService* download_service,
      scoped_refptr<base::SequencedTaskRunner> background_task_runner);
  virtual ~AdBlockSubscriptionDownloadManager();
  AdBlockSubscriptionDownloadManager(
      const AdBlockSubscriptionDownloadManager&) = delete;
  AdBlockSubscriptionDownloadManager& operator=(
      const AdBlockSubscriptionDownloadManager&) = delete;

  // Starts a download for |download_url|. Will schedule a higher priority
  // download if |from_ui| is true.
  virtual void StartDownload(const GURL& download_url, bool from_ui);

  // Cancels all pending downloads.
  virtual void CancelAllPendingDownloads();

  // Returns whether the downloader can be used for downloads.
  virtual bool IsAvailableForDownloads() const;

 private:
  friend class AdBlockSubscriptionDownloadClient;

  // Invoked when the Download Service is ready.
  //
  // |pending_download_guids| is the set of GUIDs that were previously scheduled
  // to be downloaded and have still not been downloaded yet.
  // |successful_downloads| is the map from GUID to the file path that it was
  // successfully downloaded to.
  void OnDownloadServiceReady(
      const std::set<std::string>& pending_download_guids,
      const std::map<std::string, base::FilePath>& successful_downloads);

  // Invoked when the Download Service fails to initialize and should not be
  // used for the session.
  void OnDownloadServiceUnavailable();

  // Invoked when the download has been accepted and persisted by the
  // DownloadService.
  void OnDownloadStarted(const GURL download_url,
                         const std::string& guid,
                         download::DownloadParams::StartResult start_result);

  // Invoked when the download as specified by |downloaded_guid| succeeded.
  void OnDownloadSucceeded(
      const std::string& downloaded_guid,
      std::unique_ptr<storage::BlobDataHandle> data_handle);

  // Invoked when the download as specified by |failed_download_guid| failed.
  void OnDownloadFailed(const std::string& failed_download_guid);

  // GUIDs that are still pending download, mapped to the corresponding URLs of
  // their subscription services.
  std::map<std::string, GURL> pending_download_guids_;

  // The Download Service to schedule list downloads with.
  //
  // Guaranteed to outlive |this|.
  download::DownloadService* download_service_;

  // Whether the download service is available.
  bool is_available_for_downloads_;

  // Background thread where download file processing should be performed.
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  // Sequence checker used to verify all public API methods are called on the
  // UI thread.
  SEQUENCE_CHECKER(sequence_checker_);

  // Used to get weak ptr to self on the UI thread.
  base::WeakPtrFactory<AdBlockSubscriptionDownloadManager> ui_weak_ptr_factory_{
      this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_DOWNLOAD_MANAGER_H_
