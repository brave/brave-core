// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_ads/brave_ads_host_android.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_ads/android/jni_headers/BraveAdsHostAndroid_jni.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/profiles/profile.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;

namespace brave_ads {

BraveAdsHostAndroid::BraveAdsHostAndroid(Profile* profile) : profile_(profile) {
  DCHECK(profile_);

  java_object_.Reset(Java_BraveAdsHostAndroid_create(
      AttachCurrentThread(), reinterpret_cast<intptr_t>(this)));
}

BraveAdsHostAndroid::~BraveAdsHostAndroid() {
  Java_BraveAdsHostAndroid_destroy(AttachCurrentThread(), java_object_);
}

void BraveAdsHostAndroid::RequestAdsEnabled(
    RequestAdsEnabledCallback callback) {
  const AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
  brave_rewards::RewardsService* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  if (!rewards_service || !ads_service || !ads_service->IsSupportedLocale()) {
    std::move(callback).Run(false);
    return;
  }

  if (ads_service->IsEnabled()) {
    std::move(callback).Run(true);
    return;
  }

  if (!callbacks_.empty()) {
    callbacks_.push_back(std::move(callback));
    return;
  }

  callbacks_.push_back(std::move(callback));

  rewards_service_observation_.Observe(rewards_service);

  Java_BraveAdsHostAndroid_openBraveTalkOptInPopup(AttachCurrentThread(),
                                                   java_object_);
}

void BraveAdsHostAndroid::OnAdsEnabled(
    brave_rewards::RewardsService* rewards_service,
    bool ads_enabled) {
  DCHECK(rewards_service);

  RunCallbacksAndReset(ads_enabled);
}

void BraveAdsHostAndroid::NotifyAdsEnabled(JNIEnv* env,
                                           const JavaParamRef<jobject>& obj) {
  DCHECK(!callbacks_.empty());

  brave_rewards::RewardsService* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  if (!rewards_service) {
    RunCallbacksAndReset(false);
    return;
  }

  rewards_service->EnableRewards();
  ads_opted_in_ = true;
}

void BraveAdsHostAndroid::NotifyPopupClosed(JNIEnv* env,
                                            const JavaParamRef<jobject>& obj) {
  if (callbacks_.empty()) {
    return;
  }

  // If ads were opted-in then do nothing and wait for ads enabled event.
  if (!ads_opted_in_) {
    RunCallbacksAndReset(false);
  }
}

void BraveAdsHostAndroid::RunCallbacksAndReset(bool ads_enabled) {
  DCHECK(!callbacks_.empty());

  ads_opted_in_ = false;
  rewards_service_observation_.Reset();

  for (auto& callback : callbacks_) {
    std::move(callback).Run(ads_enabled);
  }
  callbacks_.clear();
}

}  // namespace brave_ads
