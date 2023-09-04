/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/ntp_background_images/ntp_background_image.h"

#include "base/files/file_path.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface NTPBackgroundImage ()
@property(nonatomic, copy) NSURL* imagePath;
@property(nonatomic, copy) NSString* author;
@property(nonatomic, copy) NSURL* link;
@end

@implementation NTPBackgroundImage

- (instancetype)initWithImagePath:(NSURL*)imagePath
                           author:(NSString*)author
                             link:(NSURL*)link {
  if ((self = [super init])) {
    self.imagePath = imagePath;
    self.author = author;
    self.link = link;
  }
  return self;
}

- (instancetype)initWithBackground:
    (const ntp_background_images::Background&)background {
  auto imagePath = [NSURL
      fileURLWithPath:base::SysUTF8ToNSString(background.image_file.value())];
  auto author = base::SysUTF8ToNSString(background.author);
  auto link = [NSURL URLWithString:base::SysUTF8ToNSString(background.link)];
  return [self initWithImagePath:imagePath author:author link:link];
}

@end
