/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/download/background_download_service_factory.h"

#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_download_client.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_service_manager.h"
#include "chrome/browser/download/deferred_client_wrapper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/transition_manager/full_browser_transition_manager.h"
#include "components/download/content/factory/download_service_factory_helper.h"
#include "components/download/public/background_service/blob_context_getter_factory.h"

namespace {

std::unique_ptr<download::Client> CreateAdBlockSubscriptionDownloadClient(
    Profile* profile) {
  return std::make_unique<brave_shields::AdBlockSubscriptionDownloadClient>(
      g_brave_browser_process->ad_block_service()
          ->subscription_service_manager());
}

void OnProfileCreated(download::URLLoaderFactoryGetterCallback callback,
                      Profile* profile) {
  DCHECK(callback);
  std::move(callback).Run(profile->GetURLLoaderFactory());
}

class URLLoaderFactoryGetter : public download::URLLoaderFactoryGetter {
 public:
  explicit URLLoaderFactoryGetter(SimpleFactoryKey* key) : key_(key) {
    DCHECK(key_);
  }

  URLLoaderFactoryGetter(const URLLoaderFactoryGetter&) = delete;
  URLLoaderFactoryGetter& operator=(const URLLoaderFactoryGetter&) = delete;

  ~URLLoaderFactoryGetter() override = default;

 private:
  void RetrieveURLLoader(
      download::URLLoaderFactoryGetterCallback callback) override {
    FullBrowserTransitionManager::Get()->RegisterCallbackOnProfileCreation(
        key_, base::BindOnce(&OnProfileCreated, std::move(callback)));
  }

  raw_ptr<SimpleFactoryKey> key_;
};

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

  return BuildInMemoryDownloadServiceIsolated(
      simple_factory_key, std::move(clients), network_connection_tracker,
      storage_dir, std::move(blob_context_getter_factory), io_task_runner,
      std::make_unique<::URLLoaderFactoryGetter>(simple_factory_key));
}

}  // namespace download

#define BuildDownloadService BuildDownloadServiceOverride
#define BuildInMemoryDownloadService BuildInMemoryDownloadServiceOverride
#include "src/chrome/browser/download/background_download_service_factory.cc"
#undef BuildInMemoryDownloadService
#undef BuildDownloadService
