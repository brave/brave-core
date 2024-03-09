/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/favicon/brave_ios_favicon_loader.h"

#import <UIKit/UIKit.h>

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "base/strings/sys_string_conversions.h"
#include "components/favicon/core/fallback_url_util.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon/core/large_icon_worker.h"
#include "components/favicon_base/fallback_icon_style.h"
#include "components/favicon_base/favicon_callback.h"
#include "components/favicon_base/favicon_types.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/chrome/browser/favicon/model/favicon_service_factory.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/common/ui/favicon/favicon_attributes.h"
#include "ios/web/public/thread/web_thread.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kFallbackIconDefaultTextColor = 0xAAAAAA;

base::CancelableTaskTracker::TaskId GetIconRawBitmapForPageUrl(
    favicon::FaviconService* favicon_service,
    const GURL& page_url,
    int min_source_size_in_pixel,
    int desired_size_in_pixel,
    favicon_base::LargeIconCallback raw_bitmap_callback,
    base::CancelableTaskTracker* tracker) {
  DCHECK(favicon_service);
  DCHECK_LE(1, min_source_size_in_pixel);
  DCHECK_LE(0, desired_size_in_pixel);

  int max_size_in_pixel =
      std::max(desired_size_in_pixel, min_source_size_in_pixel);

  static const base::NoDestructor<favicon_base::IconTypeSet> large_icon_types(
      {favicon_base::IconType::kWebManifestIcon,
       favicon_base::IconType::kFavicon, favicon_base::IconType::kTouchIcon,
       favicon_base::IconType::kTouchPrecomposedIcon});

  scoped_refptr<favicon::LargeIconWorker> worker =
      base::MakeRefCounted<favicon::LargeIconWorker>(
          min_source_size_in_pixel, desired_size_in_pixel,
          std::move(raw_bitmap_callback),
          favicon_base::LargeIconImageCallback(), tracker);

  return favicon_service->GetRawFaviconForPageURL(
      page_url, *large_icon_types, max_size_in_pixel, /*fallback_to_host=*/true,
      base::BindOnce(&favicon::LargeIconWorker::OnIconLookupComplete, worker),
      tracker);
}
}  // namespace

namespace brave_favicon {
BraveFaviconLoader::BraveFaviconLoader(favicon::FaviconService* favicon_service)
    : favicon_service_(favicon_service) {}
BraveFaviconLoader::~BraveFaviconLoader() {}

void BraveFaviconLoader::FaviconForPageUrlOrHost(
    const GURL& page_url,
    float size_in_points,
    float min_size_in_points,
    FaviconAttributesCompletionBlock faviconBlockHandler) {
  DCHECK(faviconBlockHandler);

  const CGFloat scale = UIScreen.mainScreen.scale;
  GURL block_page_url(page_url);
  auto favicon_block = ^(const favicon_base::LargeIconResult& result) {
    if (result.bitmap.is_valid()) {
      scoped_refptr<base::RefCountedMemory> data =
          result.bitmap.bitmap_data.get();

      UIImage* favicon = [UIImage
          imageWithData:[NSData dataWithBytes:data->front() length:data->size()]
                  scale:scale];
      FaviconAttributes* attributes =
          [FaviconAttributes attributesWithImage:favicon];

      DCHECK(favicon.size.width <= size_in_points &&
             favicon.size.height <= size_in_points);
      faviconBlockHandler(attributes);
      return;
    }

    // Did not fetch valid favicon
    DCHECK(result.fallback_icon_style);
    FaviconAttributes* attributes = [FaviconAttributes
        attributesWithMonogram:base::SysUTF16ToNSString(
                                   favicon::GetFallbackIconText(block_page_url))
                     textColor:UIColorFromRGB(kFallbackIconDefaultTextColor)
               backgroundColor:UIColor.clearColor
        defaultBackgroundColor:result.fallback_icon_style->
                               is_default_background_color];
    faviconBlockHandler(attributes);
  };

  // First, return a default image.
  faviconBlockHandler([FaviconAttributes attributesWithDefaultImage]);

  // Now fetch the favicon image.
  GetIconRawBitmapForPageUrl(favicon_service_, page_url,
                             scale * min_size_in_points, scale * size_in_points,
                             base::BindRepeating(favicon_block),
                             &cancelable_task_tracker_);
}

void BraveFaviconLoader::CancellAllRequests() {
  cancelable_task_tracker_.TryCancelAll();
}

base::WeakPtr<BraveFaviconLoader> BraveFaviconLoader::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}
}  // namespace brave_favicon
