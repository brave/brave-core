/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/ntp_background_images/ntp_background_images_service_ios.h"

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_observer.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/ios/browser/api/ntp_background_images/ntp_background_image+private.h"
#include "brave/ios/browser/api/ntp_background_images/ntp_sponsored_image+private.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@protocol NTPBackgroundImagesServiceObserver
@required
- (void)onUpdatedNTPBackgroundImagesData:
    (ntp_background_images::NTPBackgroundImagesData*)data;
- (void)onUpdatedNTPSponsoredImagesData:
    (ntp_background_images::NTPSponsoredImagesData*)data;
- (void)onUpdatedNTPSponsoredContent:(const base::Value::Dict&)data;
@end

class NTPBackgroundImagesServiceObserverBridge
    : public ntp_background_images::NTPBackgroundImagesService::Observer {
 public:
  explicit NTPBackgroundImagesServiceObserverBridge(
      id<NTPBackgroundImagesServiceObserver> bridge)
      : bridge_(bridge) {}

  void OnBackgroundImagesDataDidUpdate(
      ntp_background_images::NTPBackgroundImagesData* data) override {
    [bridge_ onUpdatedNTPBackgroundImagesData:data];
  }

  void OnSponsoredImagesDataDidUpdate(
      ntp_background_images::NTPSponsoredImagesData* data) override {
    [bridge_ onUpdatedNTPSponsoredImagesData:data];
  }

  void OnSponsoredContentDidUpdate(const base::Value::Dict& data) override {
    [bridge_ onUpdatedNTPSponsoredContent:data];
  }

 private:
  __weak id<NTPBackgroundImagesServiceObserver> bridge_;
};

@protocol AdsServiceObserverIOS <NSObject>
@required
- (void)onDidInitializeAdsService;
- (void)onDidClearAdsServiceData;
@end

class AdsServiceObserverBridge : public brave_ads::AdsServiceObserver {
 public:
  explicit AdsServiceObserverBridge(id<AdsServiceObserverIOS> bridge)
      : bridge_(bridge) {}

  ~AdsServiceObserverBridge() override = default;

  void OnDidInitializeAdsService() override {
    [bridge_ onDidInitializeAdsService];
  }

  void OnDidClearAdsServiceData() override {
    [bridge_ onDidClearAdsServiceData];
  }

 private:
  __weak id<AdsServiceObserverIOS> bridge_;
};

@interface NTPBackgroundImagesService () <NTPBackgroundImagesServiceObserver,
                                          AdsServiceObserverIOS> {
  std::unique_ptr<ntp_background_images::NTPBackgroundImagesService> _service;
  raw_ptr<brave_ads::AdsService> _adsService;  // Not owned.
  std::unique_ptr<AdsServiceObserverBridge> _adsServiceObserverBridge;
  std::unique_ptr<NTPBackgroundImagesServiceObserverBridge> _observerBridge;
}
@end

@implementation NTPBackgroundImagesService

- (instancetype)
    initWithBackgroundImagesService:
        (std::unique_ptr<ntp_background_images::NTPBackgroundImagesService>)
            service
                        ads_service:(brave_ads::AdsService*)ads_service {
  if ((self = [super init])) {
    _service = std::move(service);
    _adsService = ads_service;
    _adsServiceObserverBridge =
        std::make_unique<AdsServiceObserverBridge>(self);
    _adsService->AddObserver(_adsServiceObserverBridge.get());
    _observerBridge =
        std::make_unique<NTPBackgroundImagesServiceObserverBridge>(self);
    _service->AddObserver(_observerBridge.get());
    _service->Init();
  }
  return self;
}

- (void)dealloc {
  _adsService->RemoveObserver(_adsServiceObserverBridge.get());
  _service->RemoveObserver(_observerBridge.get());
}

- (NSArray<NTPBackgroundImage*>*)backgroundImages {
  auto* data = _service->GetBackgroundImagesData();
  if (data == nullptr) {
    return @[];
  }
  NSMutableArray* backgrounds = [[NSMutableArray alloc] init];
  for (const auto& background : data->backgrounds) {
    [backgrounds
        addObject:[[NTPBackgroundImage alloc] initWithBackground:background]];
  }
  return [backgrounds copy];
}

- (NTPSponsoredImageData*)sponsoredImageData {
  auto* data = _service->GetSponsoredImagesData(/*supports_rich_media=*/false);
  if (data == nullptr) {
    return nil;
  }
  return [[NTPSponsoredImageData alloc] initWithData:*data];
}

- (NSInteger)initialCountToBrandedWallpaper {
  return ntp_background_images::features::kInitialCountToBrandedWallpaper.Get();
}

- (NSInteger)countToBrandedWallpaper {
  return ntp_background_images::features::kCountToBrandedWallpaper.Get();
}

- (void)updateSponsoredImageComponentIfNeeded {
  _service->MaybeCheckForSponsoredComponentUpdate();
}

- (void)onUpdatedNTPBackgroundImagesData:
    (ntp_background_images::NTPBackgroundImagesData*)data {
  if (self.backgroundImagesUpdated) {
    self.backgroundImagesUpdated();
  }
}

- (void)onUpdatedNTPSponsoredImagesData:
    (ntp_background_images::NTPSponsoredImagesData*)data {
  if (self.sponsoredImageDataUpdated) {
    NTPSponsoredImageData* wrappedData = nil;
    if (data != nullptr) {
      wrappedData = [[NTPSponsoredImageData alloc] initWithData:*data];
    }
    self.sponsoredImageDataUpdated(wrappedData);
  }
}

- (void)onUpdatedNTPSponsoredContent:(const base::Value::Dict&)data {
  if (_adsService) {
    // Since `data` contains small JSON from a CRX component, cloning it has no
    // performance impact.
    _adsService->ParseAndSaveNewTabPageAds(data.Clone(),
                                           /*intentional*/ base::DoNothing());
  }
}

- (void)onDidInitializeAdsService {
  _service->RegisterSponsoredImagesComponent();
}

- (void)onDidClearAdsServiceData {
  _service->ForceSponsoredComponentUpdate();
}

@end
