/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/download/content/factory/download_service_factory_helper.cc"

namespace download {

std::unique_ptr<BackgroundDownloadService> BuildInMemoryDownloadServiceIsolated(
    SimpleFactoryKey* simple_factory_key,
    std::unique_ptr<DownloadClientMap> clients,
    network::NetworkConnectionTracker* network_connection_tracker,
    const base::FilePath& storage_dir,
    BlobContextGetterFactoryPtr blob_context_getter_factory,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    URLLoaderFactoryGetterPtr url_loader_factory_getter) {
  auto config = Configuration::CreateFromFinch();
  auto download_factory = std::make_unique<InMemoryDownloadFactory>(
      std::move(url_loader_factory_getter), io_task_runner);
  auto driver = std::make_unique<InMemoryDownloadDriver>(
      std::move(download_factory), std::move(blob_context_getter_factory));
  auto store = std::make_unique<NoopStore>();
  auto task_scheduler = std::make_unique<EmptyTaskScheduler>();

  // TODO(xingliu): Remove |files_storage_dir| and |storage_dir| for incognito
  // mode. See https://crbug.com/810202.
  auto files_storage_dir = storage_dir.Append(kFilesStorageDir);
  auto file_monitor = std::make_unique<EmptyFileMonitor>();

  return CreateDownloadServiceInternal(
      simple_factory_key, std::move(clients), std::move(config),
      std::move(driver), std::move(store), std::move(task_scheduler),
      std::move(file_monitor), network_connection_tracker, files_storage_dir);
}

}  // namespace download
