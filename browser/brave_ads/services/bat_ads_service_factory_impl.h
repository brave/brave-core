/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_SERVICES_BAT_ADS_SERVICE_FACTORY_IMPL_H_
#define BRAVE_BROWSER_BRAVE_ADS_SERVICES_BAT_ADS_SERVICE_FACTORY_IMPL_H_

#include "base/memory/scoped_refptr.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/components/brave_ads/browser/bat_ads_service_factory.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_ads {

class BatAdsServiceLaunch;

class BatAdsServiceFactoryImpl final : public BatAdsServiceFactory {
 public:
  BatAdsServiceFactoryImpl();

  BatAdsServiceFactoryImpl(const BatAdsServiceFactoryImpl&) = delete;
  BatAdsServiceFactoryImpl& operator=(const BatAdsServiceFactoryImpl&) = delete;

  ~BatAdsServiceFactoryImpl() override;

  // BatAdsServiceFactory:
  mojo::Remote<bat_ads::mojom::BatAdsService> Launch() override;
  void Invalidate() override;

 private:
  // The dedicated thread every launch's bind runs on, shared across `Launch`
  // calls so a superseding `Invalidate` cancels by posting to the same
  // `SingleThreadTaskRunner` the new launch's bind is posted to. That shared
  // sequence is what guarantees the old cancel cannot interleave with the
  // new bind, so only one self-owned `BatAdsServiceImpl` is ever live.
  scoped_refptr<base::SingleThreadTaskRunner> launch_task_runner_;

  // The in-flight `Launch`, if any. Ref-counted instead of owned solely by
  // `this`, since it must remain valid even if `this` is destroyed while its
  // bind is still pending on `launch_task_runner_`.
  scoped_refptr<BatAdsServiceLaunch> launch_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_SERVICES_BAT_ADS_SERVICE_FACTORY_IMPL_H_
