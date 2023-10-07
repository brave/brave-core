/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_FILTER_LIST_CATALOG_ENTRY_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_FILTER_LIST_CATALOG_ENTRY_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface AdblockFilterListCatalogEntry : NSObject
@property(readonly) NSString* uuid;
@property(readonly) NSString* url;
@property(readonly) NSString* title;
@property(readonly) NSArray<NSString*>* languages;
@property(readonly) NSString* supportURL;
@property(readonly) NSString* componentId;
@property(readonly) NSString* base64PublicKey;
@property(readonly) NSString* desc;
@property(readonly) NSString* iosComponentId;
@property(readonly) NSString* iosBase64PublicKey;
- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_FILTER_LIST_CATALOG_ENTRY_H_
