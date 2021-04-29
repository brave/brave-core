/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_NETWORK_UTILS_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_NETWORK_UTILS_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace base {
class FilePath;
}  // namespace base

namespace net {
struct NetworkTrafficAnnotationTag;
}  // namespace net

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace storage {
class BlobDataBuilder;
}  // namespace storage

namespace ipfs {

void AddMultipartHeaderForUploadWithFileName(const std::string& value_name,
                                             const std::string& file_name,
                                             const std::string& absolute_path,
                                             const std::string& mime_boundary,
                                             const std::string& content_type,
                                             std::string* post_data);

std::unique_ptr<storage::BlobDataBuilder> BuildBlobWithFile(
    base::FilePath upload_file_path,
    size_t file_size,
    std::string mime_type,
    std::string filename,
    std::string mime_boundary);

int64_t CalculateFileSize(base::FilePath upload_file_path);

using BlobBuilderCallback =
    base::OnceCallback<std::unique_ptr<storage::BlobDataBuilder>()>;

std::unique_ptr<network::SimpleURLLoader> CreateURLLoader(
    const GURL& gurl,
    const std::string& method,
    std::unique_ptr<network::ResourceRequest> request = nullptr);

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_NETWORK_UTILS_H_
