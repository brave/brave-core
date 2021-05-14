/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/import/ipfs_file_import_worker.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/task_runner_util.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "url/gurl.h"

namespace ipfs {

IpfsFileImportWorker::IpfsFileImportWorker(content::BrowserContext* context,
                                           const GURL& endpoint,
                                           ImportCompletedCallback callback,
                                           const base::FilePath& path,
                                           const std::string& key)
    : IpfsImportWorkerBase(context, endpoint, std::move(callback), key),
      weak_factory_(this) {
  std::string filename = path.BaseName().MaybeAsASCII();
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()}, base::BindOnce(&CalculateFileSize, path),
      base::BindOnce(&IpfsFileImportWorker::CreateRequestWithFile,
                     weak_factory_.GetWeakPtr(), path, kFileMimeType,
                     filename));
}

IpfsFileImportWorker::~IpfsFileImportWorker() = default;

}  // namespace ipfs
