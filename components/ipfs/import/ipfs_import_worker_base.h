/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_IMPORT_WORKER_BASE_H_
#define BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_IMPORT_WORKER_BASE_H_

#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/queue.h"
#include "base/files/file_util.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/ipfs/import/import_utils.h"
#include "brave/components/ipfs/import/imported_data.h"
#include "components/version_info/channel.h"
#include "content/public/browser/browser_context.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
class ChromeBlobStorageContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
struct ResourceRequest;
}  // namespace network

namespace ipfs {

// A base class that implements steps for importing objects into ipfs.
// In order to import an object it is necessary to create
// an ImportWorker of the desired type, each worker can import only one object.
// The worker must be deleted when the import is completed.
// The import process consists of the following steps:
// Worker:
//   1. Worker prepares a blob block of data to import
// IpfsImportWorkerBase:
//   2. Sends blob to ifps using IPFS api (/api/v0/add)
//   3. Creates target directory for import using IPFS api(/api/v0/files/mkdir)
//   4. Moves objects to target directory using IPFS api(/api/v0/files/cp)
class IpfsImportWorkerBase {
 public:
  IpfsImportWorkerBase(content::BrowserContext* context,
                       const GURL& endpoint,
                       ImportCompletedCallback callback);
  virtual ~IpfsImportWorkerBase();

  IpfsImportWorkerBase(const IpfsImportWorkerBase&) = delete;
  IpfsImportWorkerBase& operator=(const IpfsImportWorkerBase&) = delete;

 protected:
  void StartImport(BlobBuilderCallback blob_builder_callback,
                   const std::string& content_type,
                   const std::string& filename);
  std::unique_ptr<network::SimpleURLLoader> CreateURLLoader(
      const GURL& gurl,
      const std::string& method);
  scoped_refptr<network::SharedURLLoaderFactory> GetUrlLoaderFactory();

  virtual void NotifyImportCompleted(ipfs::ImportState state);

  void CreateRequestWithFile(const base::FilePath upload_file_path,
                             const std::string& mime_type,
                             const std::string& filename,
                             int64_t file_size);

 private:
  std::unique_ptr<network::ResourceRequest> CreateResourceRequest(
      BlobBuilderCallback blob_builder_callback,
      const std::string& content_type,
      content::BrowserContext::BlobContextGetter storage_context_getter);
  // Uploading blob functions
  void UploadDataUI(std::unique_ptr<network::ResourceRequest> request);

  void OnImportAddComplete(std::unique_ptr<std::string> response_body);

  void CreateBraveDirectory();
  void OnImportDirectoryCreated(const std::string& directory,
                                std::unique_ptr<std::string> response_body);
  void CopyFilesToBraveDirectory();
  void OnImportFilesMoved(std::unique_ptr<std::string> response_body);
  bool ParseResponseBody(const std::string& response_body,
                         ipfs::ImportedData* data);
  ImportCompletedCallback callback_;
  std::unique_ptr<ipfs::ImportedData> data_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  GURL server_endpoint_;
  content::BrowserContext* browser_context_ = nullptr;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  base::WeakPtrFactory<IpfsImportWorkerBase> weak_factory_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_IMPORT_WORKER_BASE_H_
