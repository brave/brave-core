/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/import/ipfs_import_worker_base.h"

#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_json_parser.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/ipfs/service_sandbox_type.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/service_process_host.h"
#include "net/base/mime_util.h"
#include "net/base/url_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/blink/public/mojom/blob/serialized_blob.mojom.h"
#include "url/gurl.h"

namespace {

// Return a date string formatted as "YYYY-MM-DD".
std::string TimeFormatDate(const base::Time& time) {
  base::Time::Exploded exploded_time;
  time.UTCExplode(&exploded_time);
  return base::StringPrintf("%04d-%02d-%02d", exploded_time.year,
                            exploded_time.month, exploded_time.day_of_month);
}

}  // namespace

namespace ipfs {

IpfsImportWorkerBase::IpfsImportWorkerBase(
    BlobContextGetterFactory* blob_context_getter_factory,
    network::mojom::URLLoaderFactory* url_loader_factory,
    const GURL& endpoint,
    ImportCompletedCallback callback,
    const std::string& key)
    : callback_(std::move(callback)),
      blob_context_getter_factory_(blob_context_getter_factory),
      url_loader_factory_(url_loader_factory),
      server_endpoint_(endpoint),
      key_to_publish_(key),
      weak_factory_(this) {
  DCHECK(endpoint.is_valid());
  data_.reset(new ipfs::ImportedData());
}

IpfsImportWorkerBase::~IpfsImportWorkerBase() = default;

void IpfsImportWorkerBase::ImportFile(const base::FilePath path) {
  ImportFile(path, kFileMimeType, path.BaseName().MaybeAsASCII());
}

void IpfsImportWorkerBase::ImportFile(const base::FilePath upload_file_path,
                                      const std::string& mime_type,
                                      const std::string& filename) {
  data_->filename = filename;

  auto upload_callback = base::BindOnce(&IpfsImportWorkerBase::UploadData,
                                        weak_factory_.GetWeakPtr());

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ipfs::CalculateFileSize, upload_file_path),
      base::BindOnce(&CreateRequestForFile, upload_file_path,
                     blob_context_getter_factory_, mime_type, filename,
                     std::move(upload_callback)));
}

void IpfsImportWorkerBase::ImportFolder(const base::FilePath folder_path) {
  auto upload_callback = base::BindOnce(&IpfsImportWorkerBase::UploadData,
                                        weak_factory_.GetWeakPtr());
  data_->filename = folder_path.BaseName().MaybeAsASCII();
  CreateRequestForFolder(folder_path, blob_context_getter_factory_,
                         std::move(upload_callback));
}

void IpfsImportWorkerBase::ImportText(const std::string& text,
                                      const std::string& host) {
  if (text.empty() || host.empty()) {
    NotifyImportCompleted(IPFS_IMPORT_ERROR_REQUEST_EMPTY);
    return;
  }
  size_t key = base::FastHash(base::as_bytes(base::make_span(text)));
  std::string filename = host;
  filename += "_";
  filename += std::to_string(key);
  auto upload_callback = base::BindOnce(&IpfsImportWorkerBase::UploadData,
                                        weak_factory_.GetWeakPtr());
  data_->filename = filename;
  CreateRequestForText(text, filename, blob_context_getter_factory_,
                       std::move(upload_callback));
}

void IpfsImportWorkerBase::UploadData(
    std::unique_ptr<network::ResourceRequest> request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!request)
    return NotifyImportCompleted(IPFS_IMPORT_ERROR_REQUEST_EMPTY);
  if (!server_endpoint_.is_valid())
    return NotifyImportCompleted(IPFS_IMPORT_ERROR_ADD_FAILED);

  GURL url = net::AppendQueryParameter(server_endpoint_.Resolve(kImportAddPath),
                                       "stream-channels", "true");
  url = net::AppendQueryParameter(url, "wrap-with-directory", "true");
  url = net::AppendQueryParameter(url, "pin", "false");
  url = net::AppendQueryParameter(url, "progress", "false");

  DCHECK(!url_loader_);
  url_loader_ = CreateURLLoader(url, "POST", std::move(request));

  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_,
      base::BindOnce(&IpfsImportWorkerBase::OnImportAddComplete,
                     weak_factory_.GetWeakPtr()));
}

bool IpfsImportWorkerBase::ParseResponseBody(const std::string& response_body,
                                             ipfs::ImportedData* data) {
  DCHECK(data);
  std::vector<std::string> parts = base::SplitString(
      response_body, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (!parts.size())
    return IPFSJSONParser::GetImportResponseFromJSON(response_body, data);
  for (const auto& item : parts) {
    if (item.front() != '{' || item.back() != '}')
      continue;
    ipfs::ImportedData imported_item;
    if (IPFSJSONParser::GetImportResponseFromJSON(item, &imported_item)) {
      if (imported_item.filename != data->filename)
        continue;
      data->hash = imported_item.hash;
      data->size = imported_item.size;
      return true;
    }
  }
  return false;
}

void IpfsImportWorkerBase::OnImportAddComplete(
    std::unique_ptr<std::string> response_body) {
  int error_code = url_loader_->NetError();
  int response_code = -1;
  if (url_loader_->ResponseInfo() && url_loader_->ResponseInfo()->headers)
    response_code = url_loader_->ResponseInfo()->headers->response_code();

  bool success = (error_code == net::OK && response_code == net::HTTP_OK);
  if (success) {
    success = ParseResponseBody(*response_body, data_.get());
  }
  url_loader_.reset();
  if (success && !data_->hash.empty()) {
    CreateBraveDirectory();
    return;
  }
  NotifyImportCompleted(IPFS_IMPORT_ERROR_ADD_FAILED);
}

void IpfsImportWorkerBase::CreateBraveDirectory() {
  DCHECK(!url_loader_);
  GURL url = net::AppendQueryParameter(
      server_endpoint_.Resolve(kImportMakeDirectoryPath), "parents", "true");
  std::string directory = kImportDirectory;
  directory += TimeFormatDate(base::Time::Now());
  directory += "/";
  url = net::AppendQueryParameter(url, "arg", directory);

  url_loader_ = CreateURLLoader(url, "POST");
  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_,
      base::BindOnce(&IpfsImportWorkerBase::OnImportDirectoryCreated,
                     base::Unretained(this), directory));
}

void IpfsImportWorkerBase::OnImportDirectoryCreated(
    const std::string& directory,
    std::unique_ptr<std::string> response_body) {
  int error_code = url_loader_->NetError();
  int response_code = -1;
  if (url_loader_->ResponseInfo() && url_loader_->ResponseInfo()->headers)
    response_code = url_loader_->ResponseInfo()->headers->response_code();
  url_loader_.reset();
  bool success = (error_code == net::OK && response_code == net::HTTP_OK);
  if (success) {
    data_->directory = directory;
    CopyFilesToBraveDirectory();
    return;
  }
  NotifyImportCompleted(IPFS_IMPORT_ERROR_MKDIR_FAILED);
}

void IpfsImportWorkerBase::CopyFilesToBraveDirectory() {
  DCHECK(!url_loader_);
  std::string from = "/ipfs/" + data_->hash;
  GURL url = net::AppendQueryParameter(
      server_endpoint_.Resolve(kImportCopyPath), "arg", from);
  std::string to = data_->directory + "/" + data_->filename;
  url = net::AppendQueryParameter(url, "arg", to);

  url_loader_ = CreateURLLoader(url, "POST");
  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_,
      base::BindOnce(&IpfsImportWorkerBase::OnImportFilesMoved,
                     base::Unretained(this)));
}

void IpfsImportWorkerBase::OnImportFilesMoved(
    std::unique_ptr<std::string> response_body) {
  int error_code = url_loader_->NetError();
  int response_code = -1;
  if (url_loader_->ResponseInfo() && url_loader_->ResponseInfo()->headers)
    response_code = url_loader_->ResponseInfo()->headers->response_code();
  url_loader_.reset();
  bool success = (error_code == net::OK && response_code == net::HTTP_OK);
  if (!success) {
    VLOG(1) << "error_code:" << error_code << " response_code:" << response_code
            << " response_body:" << *response_body;
  }
  if (!data_->hash.empty() && !key_to_publish_.empty()) {
    PublishContent();
    return;
  }
  NotifyImportCompleted(success ? IPFS_IMPORT_SUCCESS
                                : IPFS_IMPORT_ERROR_MOVE_FAILED);
}

void IpfsImportWorkerBase::PublishContent() {
  DCHECK(!url_loader_);
  std::string from = "/ipfs/" + data_->hash;
  GURL url = net::AppendQueryParameter(
      server_endpoint_.Resolve(kAPIPublishNameEndpoint), "arg", from);
  url = net::AppendQueryParameter(url, "key", key_to_publish_);

  url_loader_ = CreateURLLoader(url, "POST");
  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_,
      base::BindOnce(&IpfsImportWorkerBase::OnContentPublished,
                     base::Unretained(this)));
}

void IpfsImportWorkerBase::OnContentPublished(
    std::unique_ptr<std::string> response_body) {
  int error_code = url_loader_->NetError();
  int response_code = -1;
  if (url_loader_->ResponseInfo() && url_loader_->ResponseInfo()->headers)
    response_code = url_loader_->ResponseInfo()->headers->response_code();
  url_loader_.reset();
  bool success = (error_code == net::OK && response_code == net::HTTP_OK);
  if (success)
    data_->published_key = key_to_publish_;
  if (!success) {
    VLOG(1) << "error_code:" << error_code << " response_code:" << response_code
            << " response_body:" << *response_body;
  }

  NotifyImportCompleted(success ? IPFS_IMPORT_SUCCESS
                                : IPFS_IMPORT_ERROR_PUBLISH_FAILED);
}

void IpfsImportWorkerBase::NotifyImportCompleted(ipfs::ImportState state) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  data_->state = state;
  if (callback_)
    std::move(callback_).Run(*data_.get());
}

network::mojom::URLLoaderFactory* IpfsImportWorkerBase::GetUrlLoaderFactory() {
  return url_loader_factory_;
}

}  // namespace ipfs
