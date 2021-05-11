/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/import/ipfs_text_import_worker.h"

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "net/base/mime_util.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "url/gurl.h"

namespace ipfs {

IpfsTextImportWorker::IpfsTextImportWorker(content::BrowserContext* context,
                                           const GURL& endpoint,
                                           ImportCompletedCallback callback,
                                           const std::string& text,
<<<<<<< HEAD
<<<<<<< HEAD
                                           const std::string& host)
    : IpfsImportWorkerBase(context, endpoint, std::move(callback)) {
  ImportText(text, host);
<<<<<<< HEAD
=======
                                           const std::string& host,
                                           const std::string& key)
    : IpfsImportWorkerBase(context, endpoint, std::move(callback), key) {
=======
                                           const std::string& host)
    : IpfsImportWorkerBase(context, endpoint, std::move(callback)) {
>>>>>>> 935de1db9f (Remove keys submenu for tabs)
  StartImportText(text, host);
>>>>>>> 7fb47a018f (Added IPNS keys manager UI to IPFS Settings)
=======
>>>>>>> e45207ba3b (Added keys import)
}

IpfsTextImportWorker::~IpfsTextImportWorker() = default;


<<<<<<< HEAD

=======
void IpfsTextImportWorker::ImportText(const std::string& text,
                                      const std::string& host) {
  if (text.empty() || host.empty()) {
    NotifyImportCompleted(IPFS_IMPORT_ERROR_REQUEST_EMPTY);
    return;
  }
  size_t key = base::FastHash(base::as_bytes(base::make_span(text)));
  std::string filename = host;
  filename += "_";
  filename += std::to_string(key);
  auto blob_storage_context_getter =
    content::BrowserContext::GetBlobStorageContext(GetBrowserContext());

  auto upload_callback =
      base::BindOnce(&IpfsTextImportWorker::UploadData, GetWeakPtr());
  CreateRequestForText(text, filename, std::move(blob_storage_context_getter),
                         std::move(upload_callback));
}
>>>>>>> e45207ba3b (Added keys import)

}  // namespace ipfs
