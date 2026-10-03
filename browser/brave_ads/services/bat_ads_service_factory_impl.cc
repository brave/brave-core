/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/services/bat_ads_service_factory_impl.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/task/single_thread_task_runner_thread_mode.h"
#include "base/task/thread_pool.h"
#include "brave/browser/brave_ads/services/bat_ads_service_feature.h"
#include "brave/browser/brave_ads/services/bat_ads_service_launch.h"
#include "content/public/browser/browser_thread.h"

namespace brave_ads {

namespace {

// Launches an in process Bat Ads Service.
mojo::Remote<bat_ads::mojom::BatAdsService> LaunchInProcessBatAdsService(
    const scoped_refptr<base::SingleThreadTaskRunner>& launch_task_runner,
    scoped_refptr<BatAdsServiceLaunch> launch) {
  mojo::Remote<bat_ads::mojom::BatAdsService> bat_ads_service_remote;
  launch_task_runner->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BatAdsServiceLaunch::Bind, std::move(launch),
                     bat_ads_service_remote.BindNewPipeAndPassReceiver()),
      kBraveAdsServiceStartupDelay.Get());
  return bat_ads_service_remote;
}

}  // namespace

BatAdsServiceFactoryImpl::BatAdsServiceFactoryImpl() = default;

BatAdsServiceFactoryImpl::~BatAdsServiceFactoryImpl() = default;

mojo::Remote<bat_ads::mojom::BatAdsService> BatAdsServiceFactoryImpl::Launch() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Invalidate();

  launch_task_runner_ = base::ThreadPool::CreateSingleThreadTaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives()},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED);
  launch_ = base::MakeRefCounted<BatAdsServiceLaunch>();

  return LaunchInProcessBatAdsService(launch_task_runner_, launch_);
}

void BatAdsServiceFactoryImpl::Invalidate() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!launch_) {
    return;
  }

  launch_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&BatAdsServiceLaunch::Cancel, launch_));
  launch_task_runner_.reset();
  launch_.reset();
}

}  // namespace brave_ads
