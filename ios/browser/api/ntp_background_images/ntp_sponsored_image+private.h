/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_IMAGE_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_IMAGE_PRIVATE_H_

#include "brave/ios/browser/api/ntp_background_images/ntp_sponsored_image.h"

NS_ASSUME_NONNULL_BEGIN

namespace ntp_background_images {
struct NTPSponsoredImagesData;
struct Campaign;
struct SponsoredBackground;
struct Logo;
struct TopSite;
}  // namespace ntp_background_images

@interface NTPSponsoredImageData (Private)
- (instancetype)initWithData:
    (const ntp_background_images::NTPSponsoredImagesData&)data;
@end

@interface NTPSponsoredImageCampaign (Private)
- (instancetype)initWithCampaign:
    (const ntp_background_images::Campaign&)campaign;
@end

@interface NTPSponsoredImageBackground (Private)
- (instancetype)initWithSponsoredBackground:
    (const ntp_background_images::SponsoredBackground&)sponsoredBackground;
@end

@interface NTPSponsoredImageLogo (Private)
- (instancetype)initWithLogo:(const ntp_background_images::Logo&)log;
@end

@interface NTPSponsoredImageTopSite (Private)
- (instancetype)initWithTopSite:(const ntp_background_images::TopSite&)topSite;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_IMAGE_PRIVATE_H_
