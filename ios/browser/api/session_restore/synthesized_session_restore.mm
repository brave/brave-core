/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/session_restore/synthesized_session_restore.h"
#include "base/ios/ios_util.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/web/navigation/synthesized_history_entry_data.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Session history entry data keys.
NSString* const kEntryData = @"SessionHistoryEntryData";
NSString* const kEntryOriginalURL = @"SessionHistoryEntryOriginalURL";
NSString* const kEntryExternalURLPolicy =
    @"SessionHistoryEntryShouldOpenExternalURLsPolicyKey";
NSString* const kEntryTitle = @"SessionHistoryEntryTitle";
NSString* const kEntryURL = @"SessionHistoryEntryURL";

// Session history keys.
NSString* const kSessionHistory = @"SessionHistory";
NSString* const kSessionHistoryCurrentIndex = @"SessionHistoryCurrentIndex";
NSString* const kSessionHistoryEntries = @"SessionHistoryEntries";
NSString* const kSessionHistoryVersion = @"SessionHistoryVersion";
NSString* const kIsAppInitiated = @"IsAppInitiated";

}  // namespace

@implementation SynthesizedSessionRestore
+ (NSData*)serializeWithTitle:(NSString*)title
                  historyURLs:(NSArray<NSURL*>*)urls
                    pageIndex:(NSUInteger)pageIndex
            isPrivateBrowsing:(bool)isPrivateBrowsing {
  int external_url_policy = isPrivateBrowsing ? 0 : 1;
  NSMutableArray* entries =
      [[NSMutableArray alloc] initWithCapacity:[urls count]];
  for (size_t i = 0; i < [urls count]; ++i) {
    // SessionHistoryEntryData, and NSDictionaries below, come from:
    // https://github.com/WebKit/WebKit/blob/674bd0ec/Source/WebKit/UIProcess/mac/LegacySessionStateCoding.cpp
    web::SynthesizedHistoryEntryData entry_data;
    entry_data.SetReferrer(GURL());
    [entries addObject:@{
      kEntryData : entry_data.AsNSData(),
      kEntryOriginalURL :
          base::SysUTF8ToNSString(net::GURLWithNSURL(urls[i]).spec()),
      kEntryExternalURLPolicy : @(external_url_policy),
      kEntryTitle : title,
      kEntryURL : base::SysUTF8ToNSString(net::GURLWithNSURL(urls[i]).spec()),
    }];
  }

  NSDictionary* state_dictionary = @{
    kSessionHistory : @{
      kSessionHistoryCurrentIndex : @(pageIndex),
      kSessionHistoryEntries : entries,
      kSessionHistoryVersion : @1,
    },
    kIsAppInitiated : @NO,
  };

  static constexpr uint8_t version[] = {0, 0, 0, 2};
  NSMutableData* interaction_data = [NSMutableData data];
  [interaction_data appendData:[NSData dataWithBytes:&version
                                              length:sizeof(version)]];
  NSData* property_list_data = [NSPropertyListSerialization
      dataWithPropertyList:state_dictionary
                    format:NSPropertyListBinaryFormat_v1_0
                   options:0
                     error:nil];
  [interaction_data appendData:property_list_data];
  return interaction_data;
}
@end
