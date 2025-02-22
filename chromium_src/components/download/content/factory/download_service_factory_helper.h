/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_CONTENT_FACTORY_DOWNLOAD_SERVICE_FACTORY_HELPER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_CONTENT_FACTORY_DOWNLOAD_SERVICE_FACTORY_HELPER_H_

#include "src/components/download/content/factory/download_service_factory_helper.h"

namespace download {

std::unique_ptr<BackgroundDownloadService> BuildInMemoryDownloadServiceIsolated(
    SimpleFactoryKey* simple_factory_key,
    std::unique_ptr<DownloadClientMap> clients,
    network::NetworkConnectionTracker* network_connection_tracker,
    const base::FilePath& storage_dir,
    BlobContextGetterFactoryPtr blob_context_getter_factory,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    URLLoaderFactoryGetterPtr url_loader_factory_getter);

}  // namespace download

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_CONTENT_FACTORY_DOWNLOAD_SERVICE_FACTORY_HELPER_H_
