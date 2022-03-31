/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_network_utils.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/check.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/components/ipfs/blob_context_getter_factory.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "content/public/browser/browser_task_traits.h"
#include "net/base/mime_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_impl.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "third_party/blink/public/mojom/blob/serialized_blob.mojom.h"
#endif

namespace {
net::NetworkTrafficAnnotationTag GetIpfsNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ipfs_service", R"(
          semantics {
            sender: "IPFS service"
            description:
              "This service is used to communicate with IPFS daemon "
              "on behalf of the user interacting with the actions in brave://ipfs."
            trigger:
              "Triggered by actions in brave://ipfs."
            data:
              "Options of the commands."
            destination: WEBSITE
          }
          policy {
            cookies_allowed: NO
            setting:
              "You can enable or disable this feature in brave://settings."
            policy_exception_justification:
              "Not implemented."
          }
        )");
}

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
struct ImportFileInfo {
  ImportFileInfo(base::FilePath full_path,
                 base::FileEnumerator::FileInfo information) {
    path = full_path;
    info = information;
  }
  base::FilePath path;
  base::FileEnumerator::FileInfo info;
};

bool GetRelativePathComponent(const base::FilePath& parent,
                              const base::FilePath& child,
                              base::FilePath::StringType* out) {
  if (!parent.IsParent(child))
    return false;

  std::vector<base::FilePath::StringType> parent_components =
      parent.GetComponents();
  std::vector<base::FilePath::StringType> child_components =
      child.GetComponents();

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

std::unique_ptr<storage::BlobDataBuilder> BuildBlobWithFile(
    base::FilePath upload_file_path,
    std::string mime_type,
    std::string filename,
    std::string mime_boundary,
    size_t file_size) {
  auto blob_builder =
      std::make_unique<storage::BlobDataBuilder>(base::GenerateGUID());
  if (filename.empty())
    filename = upload_file_path.BaseName().MaybeAsASCII();
  std::string post_data_header;
  ipfs::AddMultipartHeaderForUploadWithFileName(ipfs::kFileValueName, filename,
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

std::unique_ptr<storage::BlobDataBuilder> BuildBlobWithFolder(
    base::FilePath upload_path,
    std::string mime_boundary,
    std::vector<ImportFileInfo> files) {
  auto blob_builder =
      std::make_unique<storage::BlobDataBuilder>(base::GenerateGUID());
  for (const auto& info : files) {
    std::string data_header;
    base::FilePath::StringType relative_path;
    GetRelativePathComponent(upload_path, info.path, &relative_path);

    std::string mime_type = info.info.IsDirectory() ? ipfs::kDirectoryMimeType
                                                    : ipfs::kFileMimeType;
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
#endif

}  // namespace

namespace ipfs {

std::unique_ptr<network::SimpleURLLoader> CreateURLLoader(
    const GURL& gurl,
    const std::string& method,
    std::unique_ptr<network::ResourceRequest> request) {
  if (!request)
    request = std::make_unique<network::ResourceRequest>();
  request->url = gurl;
  request->method = method;

  const url::Origin origin = url::Origin::Create(gurl);
  request->headers.SetHeader(net::HttpRequestHeaders::kOrigin,
                             origin.Serialize());

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetIpfsNetworkTrafficAnnotationTag());
  return url_loader;
}

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
std::unique_ptr<network::ResourceRequest> CreateResourceRequest(
    BlobBuilderCallback blob_builder_callback,
    const std::string& content_type,
    BlobContextGetterFactory* blob_context_getter_factory) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(blob_context_getter_factory);
  std::unique_ptr<storage::BlobDataBuilder> blob_builder =
      std::move(blob_builder_callback).Run();

  auto storage_context = blob_context_getter_factory->RetrieveStorageContext();
  std::unique_ptr<storage::BlobDataHandle> blob_handle =
      storage_context->AddFinishedBlob(std::move(blob_builder));

  auto blob = blink::mojom::SerializedBlob::New();
  blob->uuid = blob_handle->uuid();
  blob->size = blob_handle->size();
  storage::BlobImpl::Create(
      std::make_unique<storage::BlobDataHandle>(*blob_handle),
      blob->blob.InitWithNewPipeAndPassReceiver());
  // Use a Data Pipe to transfer the blob.
  mojo::PendingRemote<network::mojom::DataPipeGetter> data_pipe_getter_remote;
  mojo::Remote<blink::mojom::Blob> blob_remote(std::move(blob->blob));
  blob_remote->AsDataPipeGetter(
      data_pipe_getter_remote.InitWithNewPipeAndPassReceiver());

  auto request = std::make_unique<network::ResourceRequest>();
  request->request_body = new network::ResourceRequestBody();
  request->request_body->AppendDataPipe(std::move(data_pipe_getter_remote));
  request->headers.SetHeader(net::HttpRequestHeaders::kContentType,
                             content_type);
  return request;
}

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

void CreateRequestForFile(
    const base::FilePath& upload_file_path,
    ipfs::BlobContextGetterFactory* blob_context_getter_factory,
    const std::string& mime_type,
    const std::string& filename,
    ResourceRequestGetter request_callback,
    size_t file_size) {
  std::string mime_boundary = net::GenerateMimeMultipartBoundary();
  auto blob_builder_callback =
      base::BindOnce(&BuildBlobWithFile, upload_file_path, mime_type, filename,
                     mime_boundary, file_size);
  std::string content_type = ipfs::kIPFSImportMultipartContentType;
  content_type += " boundary=";
  content_type += mime_boundary;

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), content::BrowserThread::IO},
      base::BindOnce(&CreateResourceRequest, std::move(blob_builder_callback),
                     content_type, blob_context_getter_factory),
      std::move(request_callback));
}

std::vector<ImportFileInfo> EnumerateDirectoryFiles(base::FilePath dir_path) {
  std::vector<ImportFileInfo> files;
  base::FileEnumerator file_enum(
      dir_path, true,
      base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES);
  for (base::FilePath enum_path = file_enum.Next(); !enum_path.empty();
       enum_path = file_enum.Next()) {
    // Skip symlinks.
    if (base::IsLink(enum_path))
      continue;
    files.push_back(ImportFileInfo(enum_path, file_enum.GetInfo()));
  }

  return files;
}

void CreateRequestForFileList(
    ResourceRequestGetter request_callback,
    ipfs::BlobContextGetterFactory* blob_context_getter_factory,
    const base::FilePath& folder_path,
    std::vector<ImportFileInfo> files) {
  std::string mime_boundary = net::GenerateMimeMultipartBoundary();
  auto blob_builder_callback =
      base::BindOnce(&BuildBlobWithFolder, folder_path.DirName(), mime_boundary,
                     std::move(files));

  std::string content_type = ipfs::kIPFSImportMultipartContentType;
  content_type += " boundary=";
  content_type += mime_boundary;

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), content::BrowserThread::IO},
      base::BindOnce(&CreateResourceRequest, std::move(blob_builder_callback),
                     content_type, blob_context_getter_factory),
      std::move(request_callback));
}

void CreateRequestForFolder(const base::FilePath& folder_path,
                            ipfs::BlobContextGetterFactory* context_factory,
                            ResourceRequestGetter request_callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&EnumerateDirectoryFiles, folder_path),
      base::BindOnce(&CreateRequestForFileList, std::move(request_callback),
                     context_factory, folder_path));
}

void CreateRequestForText(const std::string& text,
                          const std::string& filename,
                          ipfs::BlobContextGetterFactory* context_factory,
                          ResourceRequestGetter request_callback) {
  std::string mime_boundary = net::GenerateMimeMultipartBoundary();
  auto blob_builder_callback =
      base::BindOnce(&BuildBlobWithText, text, ipfs::kIPFSImportTextMimeType,
                     filename, mime_boundary);
  std::string content_type = kIPFSImportMultipartContentType;
  content_type += " boundary=";
  content_type += mime_boundary;

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), content::BrowserThread::IO},
      base::BindOnce(&CreateResourceRequest, std::move(blob_builder_callback),
                     content_type, context_factory),
      std::move(request_callback));
}
#endif
}  // namespace ipfs
