/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_shields/adblock_filter_list_catalog_entry.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/base/mac/conversions.h"
#include "brave/components/brave_shields/browser/filter_list_catalog_entry.h"

@interface AdblockFilterListCatalogEntry ()
@property(nonatomic, copy) NSString* uuid;
@property(nonatomic, copy) NSString* url;
@property(nonatomic, copy) NSString* title;
@property(nonatomic, copy) NSArray<NSString*>* languages;
@property(nonatomic, copy) NSString* supportURL;
@property(nonatomic, copy) NSString* componentId;
@property(nonatomic, copy) NSString* base64PublicKey;
@property(nonatomic, copy) NSString* desc;
@end

@implementation AdblockFilterListCatalogEntry

- (instancetype)initWithFilterListCatalogEntry:
    (brave_shields::FilterListCatalogEntry)entry {
  if ((self = [super init])) {
    self.uuid = base::SysUTF8ToNSString(entry.uuid);
    self.url = base::SysUTF8ToNSString(entry.url);
    self.title = base::SysUTF8ToNSString(entry.title);
    self.languages = brave::vector_to_ns<std::string>(entry.langs);
    self.supportURL = base::SysUTF8ToNSString(entry.support_url);
    self.componentId = base::SysUTF8ToNSString(entry.component_id);
    self.base64PublicKey = base::SysUTF8ToNSString(entry.base64_public_key);
    self.desc = base::SysUTF8ToNSString(entry.desc);
  }
  return self;
}

- (brave_shields::FilterListCatalogEntry)entry {
  return brave_shields::FilterListCatalogEntry(
      base::SysNSStringToUTF8(self.uuid), base::SysNSStringToUTF8(self.url),
      base::SysNSStringToUTF8(self.title),
      brave::ns_to_vector<std::string>(self.languages),
      base::SysNSStringToUTF8(self.supportURL),
      base::SysNSStringToUTF8(self.componentId),
      base::SysNSStringToUTF8(self.base64PublicKey),
      base::SysNSStringToUTF8(self.desc));
}

@end
