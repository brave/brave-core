/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/services/bat_ads_service_factory_impl.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/single_thread_task_runner_thread_mode.h"
#include "base/task/thread_pool.h"
#include "brave/browser/brave_ads/services/service_sandbox_type.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/services/bat_ads/bat_ads_service_impl.h"
#include "brave/grit/brave_generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/service_process_host.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace brave_ads {

namespace {

// Binds the `receiver` to a new provider on a background task runner.
void BindInProcessBatAdsService(
    mojo::PendingReceiver<bat_ads::mojom::BatAdsService>
        bat_ads_service_pending_receiver) {
  mojo::MakeSelfOwnedReceiver(std::make_unique<bat_ads::BatAdsServiceImpl>(),
                              std::move(bat_ads_service_pending_receiver));
}

// Launches an in process Bat Ads Service.
mojo::Remote<bat_ads::mojom::BatAdsService> LaunchInProcessBatAdsService() {
  mojo::Remote<bat_ads::mojom::BatAdsService> bat_ads_service_remote;
  base::ThreadPool::CreateSingleThreadTaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives()},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED)
      ->PostTask(
          FROM_HERE,
          base::BindOnce(&BindInProcessBatAdsService,
                         bat_ads_service_remote.BindNewPipeAndPassReceiver()));
  return bat_ads_service_remote;
}

// Launches a new Bat Ads Service utility process.
mojo::Remote<bat_ads::mojom::BatAdsService> LaunchOutOfProcessBatAdsService() {
  return content::ServiceProcessHost::Launch<bat_ads::mojom::BatAdsService>(
      content::ServiceProcessHost::Options()
          .WithDisplayName(IDS_SERVICE_BAT_ADS)
          .Pass());
}

}  // namespace

BatAdsServiceFactoryImpl::BatAdsServiceFactoryImpl() = default;

BatAdsServiceFactoryImpl::~BatAdsServiceFactoryImpl() = default;

mojo::Remote<bat_ads::mojom::BatAdsService> BatAdsServiceFactoryImpl::Launch()
    const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  return ShouldLaunchAsInProcessService() ? LaunchInProcessBatAdsService()
                                          : LaunchOutOfProcessBatAdsService();
}

}  // namespace brave_ads
