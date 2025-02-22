/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_IN_MEMORY_DOWNLOAD_DRIVER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_IN_MEMORY_DOWNLOAD_DRIVER_H_

#include "components/download/internal/background_service/in_memory_download.h"

#define url_loader_factory_                                        \
  url_loader_factory_not_used;                                     \
                                                                   \
 public:                                                           \
  InMemoryDownloadFactory(                                         \
      URLLoaderFactoryGetterPtr url_loader_factory_getter,         \
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner); \
                                                                   \
 private:                                                          \
  URLLoaderFactoryGetterPtr url_loader_factory_getter_;            \
  raw_ptr<network::mojom::URLLoaderFactory> url_loader_factory_

#include "src/components/download/internal/background_service/in_memory_download_driver.h"
#undef url_loader_factory_

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_IN_MEMORY_DOWNLOAD_DRIVER_H_
