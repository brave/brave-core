/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_IN_MEMORY_DOWNLOAD_IMPL_START                         \
  if (url_loader_factory_getter_) {                                 \
    url_loader_factory_getter_->RetrieveURLLoader(                  \
        base::BindOnce(&InMemoryDownloadImpl::OnRetrievedURLLoader, \
                       weak_ptr_factory_.GetWeakPtr()));            \
    return;                                                         \
  }

#include "src/components/download/internal/background_service/in_memory_download.cc"

#undef BRAVE_IN_MEMORY_DOWNLOAD_IMPL_START

namespace download {

InMemoryDownloadImpl::InMemoryDownloadImpl(
    const std::string& guid,
    const RequestParams& request_params,
    scoped_refptr<network::ResourceRequestBody> request_body,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    Delegate* delegate,
    URLLoaderFactoryGetter* url_loader_factory_getter,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : InMemoryDownload(guid),
      url_loader_factory_getter_(url_loader_factory_getter),
      request_params_(request_params),
      request_body_(std::move(request_body)),
      traffic_annotation_(traffic_annotation),
      io_task_runner_(io_task_runner),
      delegate_(delegate),
      completion_notified_(false),
      started_(false) {
  DCHECK(!guid_.empty());
  DCHECK(delegate_);
}

void InMemoryDownloadImpl::OnRetrievedURLLoader(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  url_loader_factory_ = url_loader_factory.get();
  delegate_->RetrieveBlobContextGetter(
      base::BindOnce(&InMemoryDownloadImpl::OnRetrievedBlobContextGetter,
                     weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace download
