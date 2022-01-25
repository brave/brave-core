/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IMPORT_SAVE_PACKAGE_OBSERVER_H_
#define BRAVE_BROWSER_IPFS_IMPORT_SAVE_PACKAGE_OBSERVER_H_

#include "base/memory/raw_ptr.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/download_manager.h"

// Tracks downloading process for a pacakge and notifies when it is completed.
class SavePackageFinishedObserver : public download::DownloadItem::Observer,
                                    public content::DownloadManager::Observer {
 public:
  using SavePackageCompleted =
      base::OnceCallback<void(download::DownloadItem*)>;

  SavePackageFinishedObserver(content::DownloadManager* manager,
                              const base::FilePath& main_file_path,
                              SavePackageCompleted callback);
  ~SavePackageFinishedObserver() override;

  SavePackageFinishedObserver(const SavePackageFinishedObserver&) = delete;
  SavePackageFinishedObserver& operator=(const SavePackageFinishedObserver&) =
      delete;

  bool HasInProgressDownload(download::DownloadItem* item) const;

  // download::DownloadItem::Observer:
  void OnDownloadUpdated(download::DownloadItem* download) override;
  void OnDownloadDestroyed(download::DownloadItem* download) override;

  // DownloadManager::Observer:
  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* download) override;
  void ManagerGoingDown(content::DownloadManager* manager) override;

 private:
  raw_ptr<content::DownloadManager> download_manager_ = nullptr;
  raw_ptr<download::DownloadItem> download_ = nullptr;
  base::FilePath main_file_path_;
  SavePackageCompleted callback_;
};

#endif  // BRAVE_BROWSER_IPFS_IMPORT_SAVE_PACKAGE_OBSERVER_H_
