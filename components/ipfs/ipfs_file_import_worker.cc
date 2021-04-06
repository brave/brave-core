/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_file_import_worker.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/ipfs/import_utils.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/mime_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "url/gurl.h"

namespace {

const char kFileValueName[] = "file";

const char kDefaultMimeType[] = "application/octet-stream";

int64_t CalculateFileSize(base::FilePath upload_file_path) {
  int64_t file_size = -1;
  base::GetFileSize(upload_file_path, &file_size);
  return file_size;
}

std::unique_ptr<storage::BlobDataBuilder> BuildBlobWithFile(
    base::FilePath upload_file_path,
    size_t file_size,
    std::string mime_type,
    std::string filename,
    std::string mime_boundary) {
  auto blob_builder =
      std::make_unique<storage::BlobDataBuilder>(base::GenerateGUID());
  if (filename.empty())
    filename = upload_file_path.BaseName().MaybeAsASCII();
  std::string post_data_header;
  ipfs::AddMultipartHeaderForUploadWithFileName(
      kFileValueName, filename, mime_boundary, mime_type, &post_data_header);
  blob_builder->AppendData(post_data_header);

  blob_builder->AppendFile(upload_file_path, /* offset= */ 0, file_size,
                           /* expected_modification_time= */ base::Time());
  std::string post_data_footer = "\r\n";
  net::AddMultipartFinalDelimiterForUpload(mime_boundary, &post_data_footer);
  blob_builder->AppendData(post_data_footer);

  return blob_builder;
}

}  // namespace

namespace ipfs {

IpfsFileImportWorker::IpfsFileImportWorker(content::BrowserContext* context,
                                           const GURL& endpoint,
                                           ImportCompletedCallback callback,
                                           const base::FilePath& path)
    : IpfsImportWorkerBase(context, endpoint, std::move(callback)),
      weak_factory_(this) {
  StartImportFile(path);
}

IpfsFileImportWorker::~IpfsFileImportWorker() = default;

void IpfsFileImportWorker::StartImportFile(const base::FilePath& path) {
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&CalculateFileSize, path),
      base::BindOnce(&IpfsFileImportWorker::CreateRequestWithFile,
                     weak_factory_.GetWeakPtr(), path, kDefaultMimeType));
}

void IpfsFileImportWorker::CreateRequestWithFile(
    const base::FilePath upload_file_path,
    const std::string& mime_type,
    int64_t file_size) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::string filename = upload_file_path.BaseName().MaybeAsASCII();
  std::string mime_boundary = net::GenerateMimeMultipartBoundary();
  auto blob_builder_callback =
      base::BindOnce(&BuildBlobWithFile, upload_file_path, file_size, mime_type,
                     filename, mime_boundary);
  std::string content_type = kIPFSImportMultipartContentType;
  content_type += " boundary=";
  content_type += mime_boundary;
  StartImport(std::move(blob_builder_callback), content_type, filename);
}
}  // namespace ipfs
