/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_FILE_IMPORT_WORKER_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_FILE_IMPORT_WORKER_H_

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "brave/components/ipfs/imported_data.h"
#include "brave/components/ipfs/ipfs_import_worker_base.h"
#include "url/gurl.h"

namespace ipfs {

class IpfsFileImportWorker : public IpfsImportWorkerBase {
 public:
  IpfsFileImportWorker(content::BrowserContext* context,
                       const GURL& endpoint,
                       ImportCompletedCallback callback,
                       const base::FilePath& path);
  ~IpfsFileImportWorker() override;

 private:
  void StartImportFile(const base::FilePath& path);
  void OnImportDataAvailable(const base::FilePath path);

  void CreateRequestWithFile(const base::FilePath upload_file_path,
                             const std::string& mime_type,
                             int64_t file_size);
  void OnImportAddComplete(std::unique_ptr<std::string> response_body);

  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  base::WeakPtrFactory<IpfsFileImportWorker> weak_factory_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_FILE_IMPORT_WORKER_H_
