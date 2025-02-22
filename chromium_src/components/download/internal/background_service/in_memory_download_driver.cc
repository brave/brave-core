/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_IN_MEMORY_DOWNLOAD_FACTORY_CREATE                            \
  if (url_loader_factory_getter_) {                                        \
    return std::make_unique<InMemoryDownloadImpl>(                         \
        guid, request_params, std::move(request_body), traffic_annotation, \
        delegate, url_loader_factory_getter_.get(), io_task_runner_);      \
  }

#include "src/components/download/internal/background_service/in_memory_download_driver.cc"
#undef BRAVE_IN_MEMORY_DOWNLOAD_FACTORY_CREATE

namespace download {

InMemoryDownloadFactory::InMemoryDownloadFactory(
    URLLoaderFactoryGetterPtr url_loader_factory_getter,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : url_loader_factory_getter_(std::move(url_loader_factory_getter)),
      io_task_runner_(io_task_runner) {}

}  // namespace download
