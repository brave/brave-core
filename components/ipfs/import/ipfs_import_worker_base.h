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
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/ipfs/blob_context_getter_factory.h"
#include "brave/components/ipfs/import/imported_data.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "components/version_info/channel.h"
#include "url/gurl.h"

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
//   5. Publishes objects under passed IPNS key(/api/v0/name/publish)
class IpfsImportWorkerBase {
 public:
  IpfsImportWorkerBase(BlobContextGetterFactory* blob_context_getter_factory,
                       network::mojom::URLLoaderFactory* url_loader_factory,
                       const GURL& endpoint,
                       ImportCompletedCallback callback,
                       const std::string& key = std::string());
  virtual ~IpfsImportWorkerBase();

  IpfsImportWorkerBase(const IpfsImportWorkerBase&) = delete;
  IpfsImportWorkerBase& operator=(const IpfsImportWorkerBase&) = delete;

  void ImportFile(const base::FilePath upload_file_path);
  void ImportFile(const base::FilePath upload_file_path,
                  const std::string& mime_type,
                  const std::string& filename);
  void ImportText(const std::string& text, const std::string& host);
  void ImportFolder(const base::FilePath folder_path);

 protected:
  network::mojom::URLLoaderFactory* GetUrlLoaderFactory();

  virtual void NotifyImportCompleted(ipfs::ImportState state);

 private:
  void UploadData(std::unique_ptr<network::ResourceRequest> request);

  void OnImportAddComplete(std::unique_ptr<std::string> response_body);

  void CreateBraveDirectory();
  void OnImportDirectoryCreated(const std::string& directory,
                                std::unique_ptr<std::string> response_body);
  void CopyFilesToBraveDirectory();
  void OnImportFilesMoved(std::unique_ptr<std::string> response_body);
  bool ParseResponseBody(const std::string& response_body,
                         ipfs::ImportedData* data);
  void PublishContent();
  void OnContentPublished(std::unique_ptr<std::string> response_body);
  ImportCompletedCallback callback_;
  std::unique_ptr<ipfs::ImportedData> data_;

  BlobContextGetterFactory* blob_context_getter_factory_ = nullptr;
  raw_ptr<network::mojom::URLLoaderFactory> url_loader_factory_ = nullptr;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  GURL server_endpoint_;
  std::string key_to_publish_;
  base::WeakPtrFactory<IpfsImportWorkerBase> weak_factory_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IMPORT_IPFS_IMPORT_WORKER_BASE_H_
