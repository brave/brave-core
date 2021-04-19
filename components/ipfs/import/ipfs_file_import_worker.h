/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_FILE_IMPORT_WORKER_H_
#define BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_FILE_IMPORT_WORKER_H_

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "brave/components/ipfs/import/imported_data.h"
#include "brave/components/ipfs/import/ipfs_import_worker_base.h"
#include "url/gurl.h"

namespace ipfs {

class IpfsFileImportWorker : public IpfsImportWorkerBase {
 public:
  IpfsFileImportWorker(content::BrowserContext* context,
                       const GURL& endpoint,
                       ImportCompletedCallback callback,
                       const base::FilePath& path);
  ~IpfsFileImportWorker() override;

  IpfsFileImportWorker(const IpfsFileImportWorker&) = delete;
  IpfsFileImportWorker& operator=(const IpfsFileImportWorker&) = delete;

 private:
  base::WeakPtrFactory<IpfsFileImportWorker> weak_factory_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_FILE_IMPORT_WORKER_H_
