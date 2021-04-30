/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/import/ipfs_directory_import_worker.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/mime_util.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

const char kDirectoryMimeType[] = "application/x-directory";

bool GetRelativePathComponent(const base::FilePath& parent,
                              const base::FilePath& child,
                              base::FilePath::StringType* out) {
  if (!parent.IsParent(child))
    return false;

  std::vector<base::FilePath::StringType> parent_components;
  std::vector<base::FilePath::StringType> child_components;
  parent.GetComponents(&parent_components);
  child.GetComponents(&child_components);

  size_t i = 0;
  while (i < parent_components.size() &&
         child_components[i] == parent_components[i]) {
    ++i;
  }

  while (i < child_components.size()) {
    out->append(child_components[i]);
    if (++i < child_components.size())
      out->append(FILE_PATH_LITERAL("/"));
  }
  return true;
}

std::vector<ipfs::ImportFileInfo> EnumberateDirectoryFiles(
    base::FilePath dir_path) {
  std::vector<ipfs::ImportFileInfo> files;
  base::FileEnumerator file_enum(
      dir_path, true,
      base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES);
  for (base::FilePath enum_path = file_enum.Next(); !enum_path.empty();
       enum_path = file_enum.Next()) {
    // Skip symlinks.
    if (base::IsLink(enum_path))
      continue;
    files.push_back(ipfs::ImportFileInfo(enum_path, file_enum.GetInfo()));
  }

  return files;
}

std::unique_ptr<storage::BlobDataBuilder> BuildBlobWithFolder(
    base::FilePath upload_path,
    std::string mime_boundary,
    std::vector<ipfs::ImportFileInfo> files) {
  auto blob_builder =
      std::make_unique<storage::BlobDataBuilder>(base::GenerateGUID());
  for (const auto& info : files) {
    std::string data_header;
    base::FilePath::StringType relative_path;
    GetRelativePathComponent(upload_path, info.path, &relative_path);

    std::string mime_type =
        info.info.IsDirectory() ? kDirectoryMimeType : ipfs::kFileMimeType;
    data_header.append("\r\n");
    ipfs::AddMultipartHeaderForUploadWithFileName(
        ipfs::kFileValueName, base::FilePath(relative_path).MaybeAsASCII(),
        info.path.MaybeAsASCII(), mime_boundary, mime_type, &data_header);
    blob_builder->AppendData(data_header);
    if (mime_type == ipfs::kFileMimeType) {
      blob_builder->AppendFile(info.path, 0, info.info.GetSize(), base::Time());
    }
  }

  std::string post_data_footer = "\r\n";
  net::AddMultipartFinalDelimiterForUpload(mime_boundary, &post_data_footer);
  blob_builder->AppendData(post_data_footer);

  return blob_builder;
}

}  // namespace

namespace ipfs {

IpfsDirectoryImportWorker::IpfsDirectoryImportWorker(
    content::BrowserContext* context,
    const GURL& endpoint,
    ImportCompletedCallback callback,
    const base::FilePath& source_path)
    : IpfsImportWorkerBase(context, endpoint, std::move(callback)),
      source_path_(source_path),
      file_task_runner_(base::CreateSequencedTaskRunner(
          {base::ThreadPool(), base::MayBlock(),
           base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      weak_factory_(this) {
  std::string mime_boundary = net::GenerateMimeMultipartBoundary();
  auto blob_builder =
      std::make_unique<storage::BlobDataBuilder>(base::GenerateGUID());

  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&EnumberateDirectoryFiles, source_path_),
      base::BindOnce(&IpfsDirectoryImportWorker::CreateRequestWithFolder,
                     weak_factory_.GetWeakPtr(), mime_boundary));
}

IpfsDirectoryImportWorker::~IpfsDirectoryImportWorker() = default;

void IpfsDirectoryImportWorker::CreateRequestWithFolder(
    const std::string& mime_boundary,
    std::vector<ImportFileInfo> files) {
  auto blob_builder_callback =
      base::BindOnce(&BuildBlobWithFolder, source_path_.DirName(),
                     mime_boundary, std::move(files));

  std::string content_type = kIPFSImportMultipartContentType;
  content_type += " boundary=";
  content_type += mime_boundary;
  StartImport(std::move(blob_builder_callback), content_type,
              source_path_.BaseName().MaybeAsASCII());
}

}  // namespace ipfs
