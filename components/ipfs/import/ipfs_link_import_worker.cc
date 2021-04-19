/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/import/ipfs_link_import_worker.h"

#include <utility>

#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/ipfs/import/import_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/mime_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace {

const char kLinkMimeType[] = "text/html";

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
  DownloadLinkContent(url);
}

IpfsLinkImportWorker::~IpfsLinkImportWorker() {
  RemoveDownloadedFile();
}

void IpfsLinkImportWorker::DownloadLinkContent(const GURL& url) {
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
  std::string mime_type = kLinkMimeType;
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
  temp_file_path_ = path;
  std::string filename = import_url_.ExtractFileName();
  if (filename.empty())
    filename = import_url_.host();

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&CalculateFileSize, path),
      base::BindOnce(&IpfsLinkImportWorker::CreateRequestWithFile,
                     weak_factory_.GetWeakPtr(), path, mime_type, filename));
}

void IpfsLinkImportWorker::RemoveDownloadedFile() {
  if (!temp_file_path_.empty()) {
    base::ThreadPool::PostTask(
        FROM_HERE, {base::TaskPriority::BEST_EFFORT, base::MayBlock()},
        base::BindOnce(base::GetDeleteFileCallback(), temp_file_path_));
    temp_file_path_ = base::FilePath();
  }
}

void IpfsLinkImportWorker::NotifyImportCompleted(ipfs::ImportState state) {
  RemoveDownloadedFile();
  IpfsImportWorkerBase::NotifyImportCompleted(state);
}
}  // namespace ipfs
