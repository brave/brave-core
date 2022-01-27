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
#include <utility>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/download/public/background_service/download_params.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace download {
class BackgroundDownloadService;
}  // namespace download

namespace brave_shields {

class AdBlockSubscriptionServiceManager;

class AdBlockSubscriptionDownloadClient;

// Manages the downloads of filter lists for custom subscriptions.
class AdBlockSubscriptionDownloadManager
    : public KeyedService,
      public base::SupportsWeakPtr<AdBlockSubscriptionDownloadManager> {
 public:
  using DownloadManagerGetter = base::OnceCallback<void(
      base::OnceCallback<void(AdBlockSubscriptionDownloadManager*)>)>;

  AdBlockSubscriptionDownloadManager(
      download::BackgroundDownloadService* download_service,
      scoped_refptr<base::SequencedTaskRunner> background_task_runner);
  ~AdBlockSubscriptionDownloadManager() override;
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

  void set_subscription_path_callback(
      base::RepeatingCallback<base::FilePath(const GURL&)>
          subscription_path_callback) {
    subscription_path_callback_ = subscription_path_callback;
  }

  void set_on_download_succeeded_callback(
      base::RepeatingCallback<void(const GURL&)>
          on_download_succeeded_callback) {
    on_download_succeeded_callback_ = on_download_succeeded_callback;
  }

  void set_on_download_failed_callback(
      base::RepeatingCallback<void(const GURL&)> on_download_failed_callback) {
    on_download_failed_callback_ = on_download_failed_callback;
  }

 private:
  friend class AdBlockSubscriptionDownloadClient;

  bool service_ready_;

  // KeyedService implementation
  void Shutdown() override;

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
  void OnDownloadSucceeded(const std::string& downloaded_guid,
                           base::FilePath downloaded_file);

  // Invoked when the download as specified by |failed_download_guid| failed.
  void OnDownloadFailed(const std::string& failed_download_guid);

  void OnDirCreated(base::FilePath downloaded_file,
                    const GURL& download_url,
                    bool created);

  // Invoked after ReplaceFile to report the status of moving the temporary
  // download file to its destination path.
  void ReplaceFileCallback(const GURL& download_url, bool success);

  // GUIDs that are still pending download, mapped to the corresponding URLs of
  // their subscription services.
  std::map<std::string, GURL> pending_download_guids_;

  // The Download Service to schedule list downloads with.
  //
  // Guaranteed to outlive |this|.
  raw_ptr<download::BackgroundDownloadService> download_service_ = nullptr;

  // Whether the download service is available.
  bool is_available_for_downloads_;

  // Background thread where download file processing should be performed.
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  // Sequence checker used to verify all public API methods are called on the
  // UI thread.
  SEQUENCE_CHECKER(sequence_checker_);

  // Will be notified of success or failure of downloads.
  raw_ptr<AdBlockSubscriptionServiceManager> subscription_manager_ =
      nullptr;  // NOT OWNED

  base::RepeatingCallback<base::FilePath(const GURL&)>
      subscription_path_callback_;
  base::RepeatingCallback<void(const GURL&)> on_download_succeeded_callback_;
  base::RepeatingCallback<void(const GURL&)> on_download_failed_callback_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_DOWNLOAD_MANAGER_H_
