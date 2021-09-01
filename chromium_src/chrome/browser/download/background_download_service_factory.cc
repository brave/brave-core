/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/download/background_download_service_factory.h"

#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_download_client.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service_manager.h"
#include "chrome/browser/download/deferred_client_wrapper.h"
#include "components/download/content/factory/download_service_factory_helper.h"

namespace {

std::unique_ptr<download::Client> CreateAdBlockSubscriptionDownloadClient(
    Profile* profile) {
  return std::make_unique<brave_shields::AdBlockSubscriptionDownloadClient>(
      g_brave_browser_process->ad_block_service()
          ->subscription_service_manager());
}

}  // namespace

namespace download {

std::unique_ptr<BackgroundDownloadService> BuildDownloadServiceOverride(
    SimpleFactoryKey* simple_factory_key,
    std::unique_ptr<DownloadClientMap> clients,
    network::NetworkConnectionTracker* network_connection_tracker,
    const base::FilePath& storage_dir,
    SimpleDownloadManagerCoordinator* download_manager_coordinator,
    leveldb_proto::ProtoDatabaseProvider* proto_db_provider,
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    std::unique_ptr<TaskScheduler> task_scheduler) {
  clients->insert(std::make_pair(
      download::DownloadClient::CUSTOM_LIST_SUBSCRIPTIONS,
      std::make_unique<download::DeferredClientWrapper>(
          base::BindOnce(&CreateAdBlockSubscriptionDownloadClient),
          simple_factory_key)));

  return BuildDownloadService(
      simple_factory_key, std::move(clients), network_connection_tracker,
      storage_dir, download_manager_coordinator, std::move(proto_db_provider),
      background_task_runner, std::move(task_scheduler));
}

// Create download service for incognito mode without any database or file IO.
std::unique_ptr<BackgroundDownloadService> BuildInMemoryDownloadServiceOverride(
    SimpleFactoryKey* simple_factory_key,
    std::unique_ptr<DownloadClientMap> clients,
    network::NetworkConnectionTracker* network_connection_tracker,
    const base::FilePath& storage_dir,
    BlobContextGetterFactoryPtr blob_context_getter_factory,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  clients->insert(std::make_pair(
      download::DownloadClient::CUSTOM_LIST_SUBSCRIPTIONS,
      std::make_unique<download::DeferredClientWrapper>(
          base::BindOnce(&CreateAdBlockSubscriptionDownloadClient),
          simple_factory_key)));

  return BuildInMemoryDownloadService(simple_factory_key, std::move(clients),
                                      network_connection_tracker, storage_dir,
                                      std::move(blob_context_getter_factory),
                                      io_task_runner, url_loader_factory);
}

}  // namespace download

#define BuildDownloadService BuildDownloadServiceOverride
#define BuildInMemoryDownloadService BuildInMemoryDownloadServiceOverride
#include "../../../../../chrome/browser/download/background_download_service_factory.cc"
