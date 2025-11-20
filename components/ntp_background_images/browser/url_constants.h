/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_URL_CONSTANTS_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_URL_CONSTANTS_H_

namespace ntp_background_images {

inline constexpr char kBackgroundWallpaperHost[] = "background-wallpaper";
inline constexpr char kBrandedWallpaperHost[] = "branded-wallpaper";
inline constexpr char kSponsoredImagesPath[] = "sponsored-images/";

inline constexpr char kCustomWallpaperHost[] = "custom-wallpaper";
inline constexpr char kCustomWallpaperURL[] = "chrome://custom-wallpaper/";

inline constexpr char kCampaignsKey[] = "campaigns";

inline constexpr char kLogoKey[] = "logo";

inline constexpr char kCampaignIdKey[] = "campaignId";
inline constexpr char kCampaignMetricsKey[] = "metrics";

inline constexpr char kImagesKey[] = "images";
inline constexpr char kIsBackgroundKey[] = "isBackground";
inline constexpr char kImageSourceKey[] = "source";
inline constexpr char kImageAuthorKey[] = "author";
inline constexpr char kImageLinkKey[] = "link";

inline constexpr char kDestinationURLKey[] = "destinationUrl";
inline constexpr char kCompanyNameKey[] = "companyName";

inline constexpr char kCreativeInstanceIDKey[] = "creativeInstanceId";
inline constexpr char kWallpaperIDKey[] = "wallpaperId";

inline constexpr char kIsSponsoredKey[] = "isSponsored";
inline constexpr char kWallpaperURLKey[] = "wallpaperImageUrl";
inline constexpr char kWallpaperFilePathKey[] = "wallpaperImagePath";
inline constexpr char kWallpaperColorKey[] = "wallpaperColor";
inline constexpr char kWallpaperMetricTypeKey[] = "metricType";
inline constexpr char kWallpaperTypeKey[] = "type";
inline constexpr char kWallpaperRandomKey[] = "random";
inline constexpr char kWallpaperFocalPointXKey[] = "wallpaperFocalPointX";
inline constexpr char kWallpaperFocalPointYKey[] = "wallpaperFocalPointY";
inline constexpr char kImageKey[] = "image";
inline constexpr char kImagePathKey[] = "imagePath";
inline constexpr char kLogoDestinationURLPath[] = "logo.destinationUrl";
inline constexpr char kLogoImagePath[] = "logo.imagePath";

inline constexpr char kAltKey[] = "alt";

inline constexpr char kSchemaVersionKey[] = "schemaVersion";

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_URL_CONSTANTS_H_
