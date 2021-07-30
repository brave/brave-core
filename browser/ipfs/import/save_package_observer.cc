/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/import/save_package_observer.h"

#include <utility>

#include "chrome/browser/download/download_item_model.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/download_manager.h"

SavePackageFinishedObserver::SavePackageFinishedObserver(
    content::DownloadManager* manager,
    const base::FilePath& main_file_path,
    SavePackageCompleted callback)
    : download_manager_(manager),
      download_(nullptr),
      main_file_path_(main_file_path),
      callback_(std::move(callback)) {
  DCHECK(callback_);
  download_manager_->AddObserver(this);
}

SavePackageFinishedObserver::~SavePackageFinishedObserver() {
  if (download_manager_)
    download_manager_->RemoveObserver(this);

  if (download_)
    download_->RemoveObserver(this);
}

void SavePackageFinishedObserver::OnDownloadUpdated(
    download::DownloadItem* download) {
  if (download != download_)
    return;
  if (download->GetState() == download::DownloadItem::COMPLETE ||
      download->GetState() == download::DownloadItem::CANCELLED) {
    if (callback_)
      std::move(callback_).Run(download);
  }
}

void SavePackageFinishedObserver::OnDownloadDestroyed(
    download::DownloadItem* download) {
  download_->RemoveObserver(this);
  download_ = nullptr;
}

bool SavePackageFinishedObserver::HasInProgressDownload(
    download::DownloadItem* item) const {
  if (!item)
    return false;
  if (item->GetDownloadCreationType() !=
      download::DownloadItem::TYPE_SAVE_PAGE_AS)
    return false;
  return main_file_path_ == item->GetTargetFilePath();
}

void SavePackageFinishedObserver::OnDownloadCreated(
    content::DownloadManager* manager,
    download::DownloadItem* download) {
  if (!download)
    return;
  if (download->GetDownloadCreationType() !=
      download::DownloadItem::TYPE_SAVE_PAGE_AS)
    return;
  DownloadItemModel(download).SetShouldShowInShelf(false);
  if (download_)
    return;
  download_ = download;
  download->AddObserver(this);
}

void SavePackageFinishedObserver::ManagerGoingDown(
    content::DownloadManager* manager) {
  download_->RemoveObserver(this);
  download_ = nullptr;
  download_manager_->RemoveObserver(this);
  download_manager_ = nullptr;
}
