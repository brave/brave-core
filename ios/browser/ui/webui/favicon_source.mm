// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/favicon_source.h"

#include <cmath>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/hash/hash.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/favicon/brave_ios_favicon_loader.h"
#include "brave/ios/browser/favicon/brave_ios_favicon_loader_factory.h"
#include "components/favicon/core/fallback_url_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/history/core/browser/top_sites.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/chrome/browser/favicon/model/favicon_service_factory.h"
#include "ios/chrome/browser/favicon/model/ios_chrome_large_icon_service_factory.h"
#include "ios/chrome/browser/history/model/top_sites_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "ios/chrome/common/ui/favicon/favicon_constants.h"
#include "ios/web/public/thread/web_thread.h"
#include "skia/ext/skia_utils_ios.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_scale_factor.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/native_theme/native_theme.h"
#include "ui/resources/grit/ui_resources.h"
#include "url/gurl.h"

namespace chrome {
inline constexpr char kChromeUIFavicon2Host[] = "favicon2";
inline constexpr char kChromeUIFaviconHost[] = "favicon";
inline constexpr char kChromeUIUntrustedFavicon2URL[] =
    "chrome-untrusted://favicon2/";
}  // namespace chrome

namespace {

// Name of histogram to track whether the default response was returned.
const char kDefaultResponseHistogramName[] = "Favicons.DefaultResponse";

// Generous cap to guard against out-of-memory issues.
constexpr int kMaxDesiredSizeInPixel = 2048;

}  // namespace

namespace favicon {

SkBitmap GenerateMonogramFavicon(const GURL& url, float width, float height) {
  const CGSize imageSize = CGSizeMake(width, height);
  const CGFloat padding = 28.0;

  auto createBackgroundColor = [](const GURL& url) -> UIColor* {
    std::int32_t hash = base::PersistentHash(url.host());
    if (!hash) {
      return [UIColor grayColor];
    }

    static constexpr auto defaultColors = std::to_array<std::uint32_t>({
        0x2e761a, 0x399320, 0x40a624, 0x57bd35, 0x70cf5b, 0x90e07f, 0xb1eea5,
        0x881606, 0xaa1b08, 0xc21f09, 0xd92215, 0xee4b36, 0xf67964, 0xffa792,
        0x025295, 0x0568ba, 0x0675d3, 0x0996f8, 0x2ea3ff, 0x61b4ff, 0x95cdff,
        0x00736f, 0x01908b, 0x01a39d, 0x01bdad, 0x27d9d2, 0x58e7e6, 0x89f4f5,
        0xc84510, 0xe35b0f, 0xf77100, 0xff9216, 0xffad2e, 0xffc446, 0xffdf81,
        0x911a2e, 0xb7223b, 0xcf2743, 0xea385e, 0xfa526e, 0xff7a8d, 0xffa7b3,
    });

    NSInteger index = labs(hash) % (defaultColors.size() - 1);
    NSUInteger colorHex = defaultColors[index];

    CGFloat red = ((colorHex >> 16) & 0xFF) / 255.0;
    CGFloat green = ((colorHex >> 8) & 0xFF) / 255.0;
    CGFloat blue = (colorHex & 0xFF) / 255.0;

    return [UIColor colorWithRed:red green:green blue:blue alpha:1.0];
  };

  UIColor* solidTextColor = [UIColor whiteColor];
  UIColor* solidBackgroundColor = createBackgroundColor(url);

  NSString* text = base::SysUTF16ToNSString(favicon::GetFallbackIconText(url));
  UIFont* font = [UIFont systemFontOfSize:imageSize.height / 2.0];

  UIGraphicsImageRendererFormat* format =
      [UIGraphicsImageRendererFormat defaultFormat];
  UIGraphicsImageRenderer* renderer =
      [[UIGraphicsImageRenderer alloc] initWithSize:imageSize format:format];

  UIImage* finalImage =
      [renderer imageWithActions:^(UIGraphicsImageRendererContext* context) {
        CGContextRef ctx = context.CGContext;

        CGContextSetFillColorWithColor(ctx, solidBackgroundColor.CGColor);
        CGContextFillRect(ctx,
                          CGRectMake(0, 0, imageSize.width, imageSize.height));

        CGFloat fontSize = font.pointSize;
        CGSize textSize = [text sizeWithAttributes:@{
          NSFontAttributeName : [font fontWithSize:fontSize]
        }];
        if (textSize.width <= 0 || textSize.height <= 0) {
          return;
        }

        CGFloat ratio = MIN((imageSize.width - padding) / textSize.width,
                            (imageSize.height - padding) / textSize.height);
        fontSize *= ratio;

        if (fontSize < font.pointSize * 0.5) {
          fontSize = font.pointSize * 0.5;
        }

        UIFont* scaledFont = [font fontWithSize:fontSize];
        CGSize scaledTextSize =
            [text sizeWithAttributes:@{NSFontAttributeName : scaledFont}];

        CGFloat x = (imageSize.width - scaledTextSize.width) / 2.0;
        CGFloat y = (imageSize.height - scaledTextSize.height) / 2.0;

        [text drawInRect:CGRectMake(x, y, scaledTextSize.width,
                                    scaledTextSize.height)
            withAttributes:@{
              NSFontAttributeName : scaledFont,
              NSForegroundColorAttributeName : solidTextColor
            }];
      }];

  return SkBitmap(
      skia::CGImageToSkBitmap(finalImage.CGImage, finalImage.size, false));
}

}  // namespace favicon

FaviconSource::FaviconSource(ProfileIOS* profile,
                             chrome::FaviconUrlFormat url_format,
                             bool serve_untrusted)
    : profile_(profile->GetOriginalProfile()),
      url_format_(url_format),
      serve_untrusted_(serve_untrusted) {}

FaviconSource::~FaviconSource() = default;

std::string FaviconSource::GetSource() const {
  switch (url_format_) {
    case chrome::FaviconUrlFormat::kFaviconLegacy:
      return chrome::kChromeUIFaviconHost;
    case chrome::FaviconUrlFormat::kFavicon2:
      return serve_untrusted_ ? chrome::kChromeUIUntrustedFavicon2URL
                              : chrome::kChromeUIFavicon2Host;
  }

  NOTREACHED();
}

void FaviconSource::StartDataRequest(
    std::string_view path,
    web::URLDataSourceIOS::GotDataCallback callback) {
  favicon::FaviconService* favicon_service =
      ios::FaviconServiceFactory::GetForProfile(
          profile_, ServiceAccessType::EXPLICIT_ACCESS);
  if (!favicon_service) {
    SendDefaultResponse(std::move(callback));
    return;
  }

  chrome::ParsedFaviconPath parsed;
  bool success =
      chrome::ParseFaviconPath(std::string(path), url_format_, &parsed);
  if (!success) {
    SendDefaultResponse(std::move(callback));
    return;
  }

  GURL page_url(parsed.page_url);
  GURL icon_url(parsed.icon_url);
  if (!page_url.is_valid() && !icon_url.is_valid()) {
    SendDefaultResponse(std::move(callback), parsed.force_light_mode);
    return;
  }

  const int desired_size_in_pixel =
      std::ceil(parsed.size_in_dip * parsed.device_scale_factor);

  // Guard against out-of-memory issues.
  if (desired_size_in_pixel > kMaxDesiredSizeInPixel) {
    SendDefaultResponse(std::move(callback), parsed.force_light_mode);
    return;
  }

  if (parsed.page_url.empty()) {
    // Request by icon url.
    base::UmaHistogramBoolean(kDefaultResponseHistogramName, false);
    favicon_service->GetRawFavicon(
        icon_url, favicon_base::IconType::kFavicon, desired_size_in_pixel,
        base::BindOnce(&FaviconSource::OnFaviconDataAvailable,
                       base::Unretained(this), std::move(callback), parsed),
        &cancelable_task_tracker_);
  } else {
    // Intercept requests for prepopulated pages if TopSites exists.
    scoped_refptr<history::TopSites> top_sites =
        ios::TopSitesFactory::GetForProfile(profile_);
    if (top_sites) {
      for (const auto& prepopulated_page : top_sites->GetPrepopulatedPages()) {
        if (page_url == prepopulated_page.most_visited.url) {
          base::UmaHistogramBoolean(kDefaultResponseHistogramName, false);
          ui::ResourceScaleFactor resource_scale_factor =
              ui::GetSupportedResourceScaleFactor(parsed.device_scale_factor);
          std::move(callback).Run(
              ui::ResourceBundle::GetSharedInstance()
                  .LoadDataResourceBytesForScale(prepopulated_page.favicon_id,
                                                 resource_scale_factor));
          return;
        }
      }
    }

    if (!parsed.allow_favicon_server_fallback) {
      base::UmaHistogramBoolean(kDefaultResponseHistogramName, false);
      // Request from local storage only.
      const bool fallback_to_host = true;
      favicon_service->GetRawFaviconForPageURL(
          page_url, {favicon_base::IconType::kFavicon}, desired_size_in_pixel,
          fallback_to_host,
          base::BindOnce(&FaviconSource::OnFaviconDataAvailable,
                         base::Unretained(this), std::move(callback), parsed),
          &cancelable_task_tracker_);
      return;
    }

    // Request from both local storage and history
    brave_favicon::BraveFaviconLoader* brave_icon_service =
        brave_favicon::BraveIOSFaviconLoaderFactory::GetForProfile(profile_);

    if (!brave_icon_service) {
      SendDefaultResponse(std::move(callback), parsed);
      return;
    }
    base::UmaHistogramBoolean(kDefaultResponseHistogramName, false);
    brave_icon_service->RawFaviconForPageUrlOrHost(
        page_url, desired_size_in_pixel, kDesiredMediumFaviconSizePt,
        base::BindOnce(&FaviconSource::OnFaviconDataAvailable,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                       parsed));
  }
}

std::string FaviconSource::GetMimeType(std::string_view) const {
  // We need to explicitly return a mime type, otherwise if the user tries to
  // drag the image they get no extension.
  return "image/png";
}

bool FaviconSource::AllowCaching() const {
  return false;
}

bool FaviconSource::ShouldReplaceExistingSource() const {
  // Leave the existing DataSource in place, otherwise we'll drop any pending
  // requests on the floor.
  return false;
}

bool FaviconSource::ShouldServiceRequest(const GURL& url) const {
  return URLDataSourceIOS::ShouldServiceRequest(url);
}

bool FaviconSource::UseDarkMode() {
  UIUserInterfaceStyle style =
      UIScreen.mainScreen.traitCollection.userInterfaceStyle;
  return style == UIUserInterfaceStyleDark;
}

void FaviconSource::OnFaviconDataAvailable(
    web::URLDataSourceIOS::GotDataCallback callback,
    const chrome::ParsedFaviconPath& parsed,
    const favicon_base::FaviconRawBitmapResult& bitmap_result) {
  if (bitmap_result.is_valid()) {
    // Forward the data along to the networking system.
    std::move(callback).Run(bitmap_result.bitmap_data.get());
  } else {
    SendDefaultResponse(std::move(callback), parsed);
  }
}

void FaviconSource::SendDefaultResponse(
    web::URLDataSourceIOS::GotDataCallback callback,
    const chrome::ParsedFaviconPath& parsed) {
  if (!parsed.show_fallback_monogram) {
    SendDefaultResponse(std::move(callback), parsed.size_in_dip,
                        parsed.device_scale_factor,
                        parsed.force_light_mode ? false : UseDarkMode());
    return;
  }
  int icon_size = std::ceil(parsed.size_in_dip * parsed.device_scale_factor);
  SkBitmap bitmap = favicon::GenerateMonogramFavicon(GURL(parsed.page_url),
                                                     icon_size, icon_size);
  std::optional<std::vector<uint8_t>> bitmap_data =
      gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, /*discard_transparency=*/false);
  DCHECK(bitmap_data);
  std::move(callback).Run(base::MakeRefCounted<base::RefCountedBytes>(
      std::move(bitmap_data).value()));
}

void FaviconSource::SendDefaultResponse(
    web::URLDataSourceIOS::GotDataCallback callback,
    bool force_light_mode) {
  SendDefaultResponse(std::move(callback), 16, 1.0f,
                      force_light_mode ? false : UseDarkMode());
}

void FaviconSource::SendDefaultResponse(
    web::URLDataSourceIOS::GotDataCallback callback,
    int size_in_dip,
    float scale_factor,
    bool dark_mode) {
  base::UmaHistogramBoolean(kDefaultResponseHistogramName, true);
  int resource_id;
  switch (size_in_dip) {
    case 64:
      resource_id =
          dark_mode ? IDR_DEFAULT_FAVICON_DARK_64 : IDR_DEFAULT_FAVICON_64;
      break;
    case 32:
      resource_id =
          dark_mode ? IDR_DEFAULT_FAVICON_DARK_32 : IDR_DEFAULT_FAVICON_32;
      break;
    default:
      resource_id = dark_mode ? IDR_DEFAULT_FAVICON_DARK : IDR_DEFAULT_FAVICON;
      break;
  }
  std::move(callback).Run(LoadIconBytes(scale_factor, resource_id));
}

base::RefCountedMemory* FaviconSource::LoadIconBytes(float scale_factor,
                                                     int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytesForScale(
      resource_id, ui::GetSupportedResourceScaleFactor(scale_factor));
}
