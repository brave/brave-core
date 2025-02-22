/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_IN_MEMORY_DOWNLOAD_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_IN_MEMORY_DOWNLOAD_H_

#include "components/download/internal/background_service/blob_task_proxy.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#define SaveAsBlob                                                           \
  SaveAsBlobUnused();                                                        \
                                                                             \
 public:                                                                     \
  InMemoryDownloadImpl(                                                      \
      const std::string& guid, const RequestParams& request_params,          \
      scoped_refptr<network::ResourceRequestBody> request_body,              \
      const net::NetworkTrafficAnnotationTag& traffic_annotation,            \
      Delegate* delegate, URLLoaderFactoryGetter* url_loader_factory_getter, \
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);           \
                                                                             \
 private:                                                                    \
  void OnRetrievedURLLoader(scoped_refptr<network::SharedURLLoaderFactory>); \
  raw_ptr<URLLoaderFactoryGetter> url_loader_factory_getter_;                \
  void SaveAsBlob

#include "src/components/download/internal/background_service/in_memory_download.h"
#undef SaveAsBlob

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_IN_MEMORY_DOWNLOAD_H_
