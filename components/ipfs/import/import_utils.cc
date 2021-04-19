/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/import/import_utils.h"

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/check.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "net/base/mime_util.h"
#include "storage/browser/blob/blob_data_builder.h"

namespace ipfs {

void AddMultipartHeaderForUploadWithFileName(const std::string& value_name,
                                             const std::string& file_name,
                                             const std::string& absolute_path,
                                             const std::string& mime_boundary,
                                             const std::string& content_type,
                                             std::string* post_data) {
  DCHECK(post_data);
  // First line is the boundary.
  post_data->append("--" + mime_boundary + "\r\n");
  if (!absolute_path.empty())
    post_data->append("Abspath: " + absolute_path + "\r\n");
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
  ipfs::AddMultipartHeaderForUploadWithFileName(kFileValueName, filename,
                                                std::string(), mime_boundary,
                                                mime_type, &post_data_header);
  blob_builder->AppendData(post_data_header);

  blob_builder->AppendFile(upload_file_path, /* offset= */ 0, file_size,
                           /* expected_modification_time= */ base::Time());
  std::string post_data_footer = "\r\n";
  net::AddMultipartFinalDelimiterForUpload(mime_boundary, &post_data_footer);
  blob_builder->AppendData(post_data_footer);

  return blob_builder;
}

}  // namespace ipfs
