/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_link_import_worker.h"

#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/mime_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "url/gurl.h"

namespace {

const char kIPFSImportMultipartContentType[] = "multipart/form-data;";
const char kFileValueName[] = "file";

const char kDefaultMimeType[] = "text/html";

using BlobBuilderCallback =
    base::OnceCallback<std::unique_ptr<storage::BlobDataBuilder>()>;

void AddMultipartHeaderForUploadWithFileName(const std::string& value_name,
                                             const std::string& file_name,
                                             const std::string& mime_boundary,
                                             const std::string& content_type,
                                             std::string* post_data) {
  DCHECK(post_data);
  // First line is the boundary.
  post_data->append("--" + mime_boundary + "\r\n");
  // Next line is the Content-disposition.
  post_data->append("Content-Disposition: form-data; name=\"" + value_name +
                    "\"; filename=\"" + file_name + "\"\r\n");
  // If Content-type is specified, the next line is that.
  post_data->append("Content-Type: " + content_type + "\r\n");
  // Empty string before next content
  post_data->append("\r\n");
}

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
  AddMultipartHeaderForUploadWithFileName(
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

IpfsLinkImportWorker::IpfsLinkImportWorker(content::BrowserContext* context,
                                           const GURL& endpoint,
                                           ImportCompletedCallback callback,
                                           const GURL& url)
    : IpfsImportWorkerBase(context, endpoint, std::move(callback)),
      weak_factory_(this) {
  DCHECK(context);
  DCHECK(endpoint.is_valid());
  StartImportLink(url);
}

IpfsLinkImportWorker::~IpfsLinkImportWorker() = default;

void IpfsLinkImportWorker::StartImportLink(const GURL& url) {
  if (!url.is_valid()) {
    VLOG(1) << "Unable to import invalid links:" << url;
    return;
  }
  import_url_ = url;
  DCHECK(!url_loader_);
  url_loader_ = CreateURLLoader(import_url_, "GET");
  url_loader_->DownloadToTempFile(
      GetUrlLoaderFactory().get(),
      base::BindOnce(&IpfsLinkImportWorker::OnImportDataAvailable,
                     base::Unretained(this)));
}

void IpfsLinkImportWorker::OnImportDataAvailable(base::FilePath path) {
  int error_code = url_loader_->NetError();
  int response_code = -1;
  int64_t content_length = -1;
  std::string mime_type = kDefaultMimeType;
  if (url_loader_->ResponseInfo() && url_loader_->ResponseInfo()->headers) {
    response_code = url_loader_->ResponseInfo()->headers->response_code();
    content_length = url_loader_->ResponseInfo()->headers->GetContentLength();
    url_loader_->ResponseInfo()->headers->GetMimeType(&mime_type);
  }
  bool success =
      (error_code == net::OK && response_code == net::HTTP_OK && !path.empty());
  url_loader_.reset();
  if (!success) {
    VLOG(1) << "error_code:" << error_code << " response_code:" << response_code
            << " response_body:" << path.value();
    NotifyImportCompleted(IPFS_IMPORT_ERROR_REQUEST_EMPTY);
    return;
  }
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&CalculateFileSize, path),
      base::BindOnce(&IpfsLinkImportWorker::CreateRequestWithFile,
                     weak_factory_.GetWeakPtr(), path, mime_type));
}

void IpfsLinkImportWorker::CreateRequestWithFile(
    base::FilePath upload_file_path,
    const std::string& mime_type,
    int64_t file_size) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::string filename = import_url_.ExtractFileName();
  if (filename.empty())
    filename = import_url_.host();
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
