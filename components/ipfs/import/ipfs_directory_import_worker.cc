/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/import/ipfs_directory_import_worker.h"

#include <memory>
#include <string>
#include <utility>

#include "base/guid.h"
#include "content/public/browser/browser_context.h"

namespace ipfs {

IpfsDirectoryImportWorker::IpfsDirectoryImportWorker(
    content::BrowserContext* context,
    const GURL& endpoint,
    ImportCompletedCallback callback,
    const base::FilePath& source_path,
    const std::string& key)
    : IpfsImportWorkerBase(context, endpoint, std::move(callback), key),
      source_path_(source_path),
      weak_factory_(this) {

  ImportFolder(source_path);
}

IpfsDirectoryImportWorker::~IpfsDirectoryImportWorker() = default;

void IpfsDirectoryImportWorker::ImportFolder(const base::FilePath folder_path) {
  auto blob_storage_context_getter =
    content::BrowserContext::GetBlobStorageContext(GetBrowserContext());

  auto upload_callback =
      base::BindOnce(&IpfsDirectoryImportWorker::UploadData, GetWeakPtr());
  CreateRequestForFolder(folder_path, std::move(blob_storage_context_getter),
                         std::move(upload_callback));
}

}  // namespace ipfs
