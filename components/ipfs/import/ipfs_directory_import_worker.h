/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_DIRECTORY_IMPORT_WORKER_H_
#define BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_DIRECTORY_IMPORT_WORKER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/ipfs/import/imported_data.h"
#include "brave/components/ipfs/import/ipfs_import_worker_base.h"
#include "url/gurl.h"

namespace ipfs {

struct ImportFileInfo {
  ImportFileInfo(base::FilePath full_path,
                 base::FileEnumerator::FileInfo information) {
    path = full_path;
    info = information;
  }
  base::FilePath path;
  base::FileEnumerator::FileInfo info;
};

class IpfsDirectoryImportWorker : public IpfsImportWorkerBase {
 public:
  IpfsDirectoryImportWorker(content::BrowserContext* context,
                            const GURL& endpoint,
                            ImportCompletedCallback callback,
                            const base::FilePath& path);
  ~IpfsDirectoryImportWorker() override;

  IpfsDirectoryImportWorker(const IpfsDirectoryImportWorker&) = delete;
  IpfsDirectoryImportWorker& operator=(const IpfsDirectoryImportWorker&) =
      delete;

 private:
  void OnImportDataAvailable(const base::FilePath path);

  void CreateRequestWithFolder(const std::string& mime_boundary,
                               std::vector<ImportFileInfo> files);
  void OnImportAddComplete(std::unique_ptr<std::string> response_body);

  base::FilePath source_path_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  base::WeakPtrFactory<IpfsDirectoryImportWorker> weak_factory_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_DIRECTORY_IMPORT_WORKER_H_
