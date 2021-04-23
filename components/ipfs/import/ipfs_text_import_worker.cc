/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/import/ipfs_text_import_worker.h"

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/ipfs/import/import_utils.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "content/public/browser/browser_context.h"
#include "net/base/mime_util.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "url/gurl.h"

namespace {

const char kIPFSImportTextMimeType[] = "application/octet-stream";

std::unique_ptr<storage::BlobDataBuilder> BuildBlobWithText(
    const std::string& text,
    std::string mime_type,
    std::string filename,
    std::string mime_boundary) {
  auto blob_builder =
      std::make_unique<storage::BlobDataBuilder>(base::GenerateGUID());
  std::string post_data_header;
  net::AddMultipartValueForUploadWithFileName(ipfs::kFileValueName, filename,
                                              text, mime_boundary, mime_type,
                                              &post_data_header);
  blob_builder->AppendData(post_data_header);

  std::string post_data_footer = "\r\n";
  net::AddMultipartFinalDelimiterForUpload(mime_boundary, &post_data_footer);
  blob_builder->AppendData(post_data_footer);
  return blob_builder;
}

}  // namespace

namespace ipfs {

IpfsTextImportWorker::IpfsTextImportWorker(content::BrowserContext* context,
                                           const GURL& endpoint,
                                           ImportCompletedCallback callback,
                                           const std::string& text,
                                           const std::string& host)
    : IpfsImportWorkerBase(context, endpoint, std::move(callback)) {
  StartImportText(text, host);
}

IpfsTextImportWorker::~IpfsTextImportWorker() = default;

void IpfsTextImportWorker::StartImportText(const std::string& text,
                                           const std::string& host) {
  if (text.empty() || host.empty()) {
    return NotifyImportCompleted(IPFS_IMPORT_ERROR_REQUEST_EMPTY);
  }
  size_t key = base::FastHash(base::as_bytes(base::make_span(text)));
  std::string filename = host;
  filename += "_";
  filename += std::to_string(key);
  std::string mime_boundary = net::GenerateMimeMultipartBoundary();
  auto blob_builder_callback =
      base::BindOnce(&BuildBlobWithText, text, kIPFSImportTextMimeType,
                     filename, mime_boundary);
  std::string content_type = kIPFSImportMultipartContentType;
  content_type += " boundary=";
  content_type += mime_boundary;
  StartImport(std::move(blob_builder_callback), content_type, filename);
}

}  // namespace ipfs
