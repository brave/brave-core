/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/bookmarks/importer/favicon_reencode.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "base/compiler_specific.h"

namespace gfx {
// FROM: ui/gfx/geometry/size.h
constexpr int kFaviconSize = 16;
} // namespace gfx

namespace importer {

bool ReencodeFavicon(const unsigned char* src_data,
                     size_t src_len,
                     std::vector<unsigned char>* png_data) {
  if (!src_data || !png_data || src_len == 0) {
    return false;
  }
  
  // Because there is no guarantee of the mime-type of the image's `src_data`,
  // Chromium guesses the mime-type of the image using
  // ImageDecoder::CreateByMimeType & ImageDecoder::SniffMimeTypeInternal
  // Where it parses the image header to determine the type of image,
  // in order to create an SkBitmap from it. Otherwise it returns a completely RED image.
  
  // For iOS, we don't have to do that, we can let UIImage handle it.
  // If it fails, we wouldn't have been able to display such an image on iOS anyway,
  // and it'll be retrieved the next time the user pulls down the image via visiting the page.
  // OR the iOS side will handle it.
  
  //gfx::Size(gfx::kFaviconSize, gfx::kFaviconSize)
  NSData *image_data = [NSData dataWithBytes:src_data length:src_len];
  if (!image_data) {
    return false;
  }
  
  UIImage *ios_image = [UIImage imageWithData:image_data];
  if (!ios_image) {
    return false;
  }
  
  //Scale the image to the desired size: gfx::Size(gfx::kFaviconSize, gfx::kFaviconSize)
  UIGraphicsBeginImageContextWithOptions(CGSizeMake(gfx::kFaviconSize, gfx::kFaviconSize), NO, 0.0);
  [ios_image drawInRect:CGRectMake(0, 0, gfx::kFaviconSize, gfx::kFaviconSize)];
  UIImage *scaled_ios_image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  
  if (!scaled_ios_image) {
    return false;
  }
  
  NSData *ios_png_data = UIImagePNGRepresentation(scaled_ios_image);
  if (!ios_png_data || [ios_png_data length] == 0) {
    return false;
  }
  
  const unsigned char *png_bytes = static_cast<const unsigned char*>([ios_png_data bytes]);
  png_data->insert(png_data->begin(),
                   UNSAFE_TODO(png_bytes, png_bytes + [ios_png_data length]));
  return true;
}

}  // namespace importer
