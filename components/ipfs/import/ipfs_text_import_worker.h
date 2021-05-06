/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_TEXT_IMPORT_WORKER_H_
#define BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_TEXT_IMPORT_WORKER_H_

#include <string>

#include "brave/components/ipfs/import/ipfs_import_worker_base.h"
#include "url/gurl.h"

namespace ipfs {

// Implements preparation steps for importing text objects into ipfs.
// Wraps text data to a request object and puts it to the base class
// for the upload using IPFS api
class IpfsTextImportWorker : public IpfsImportWorkerBase {
 public:
  IpfsTextImportWorker(content::BrowserContext* context,
                       const GURL& endpoint,
                       ImportCompletedCallback callback,
                       const std::string& text,
                       const std::string& host);
  ~IpfsTextImportWorker() override;

  IpfsTextImportWorker(const IpfsTextImportWorker&) = delete;
  IpfsTextImportWorker& operator=(const IpfsTextImportWorker&) = delete;

 private:
  void StartImportText(const std::string& text, const std::string& host);
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_TEXT_IMPORT_WORKER_H_
