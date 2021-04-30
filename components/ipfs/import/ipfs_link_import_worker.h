/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_LINK_IMPORT_WORKER_H_
#define BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_LINK_IMPORT_WORKER_H_

#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/ipfs/import/imported_data.h"
#include "brave/components/ipfs/import/ipfs_import_worker_base.h"
#include "url/gurl.h"

namespace ipfs {

// Implements preparation steps for importing linked objects into ipfs.
// Creates a blob from downloaded data available by a link.
// Puts the blob to the base class for the upload using IPFS api
class IpfsLinkImportWorker : public IpfsImportWorkerBase {
 public:
  IpfsLinkImportWorker(content::BrowserContext* context,
                       const GURL& endpoint,
                       ImportCompletedCallback callback,
                       const GURL& url);
  ~IpfsLinkImportWorker() override;

  IpfsLinkImportWorker(const IpfsLinkImportWorker&) = delete;
  IpfsLinkImportWorker& operator=(const IpfsLinkImportWorker&) = delete;

 private:
  void DownloadLinkContent(const GURL& url);
  void OnImportDataAvailable(base::FilePath path);
  void RemoveDownloadedFile();
  // IpfsImportWorkerBase
  void NotifyImportCompleted(ipfs::ImportState state) override;

  base::FilePath temp_file_path_;
  GURL import_url_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  base::WeakPtrFactory<IpfsLinkImportWorker> weak_factory_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_LINK_IMPORT_WORKER_H_
