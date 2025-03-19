/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/variations/model/brave_variations_seed_fetcher.h"

#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "brave/base/mac/conversions.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/variations/seed_response.h"
#include "components/variations/variations_seed_store.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"

namespace {

NSString* kLastVariationsSeedFetchTimeKey = @"kLastVariationsSeedFetchTime";

base::Time GetLastVariationsSeedFetchTime() {
  double timestamp = [[NSUserDefaults standardUserDefaults]
      doubleForKey:kLastVariationsSeedFetchTimeKey];
  return base::Time::FromSecondsSinceUnixEpoch(timestamp);
}

void SaveLastVariationsSeedFetchTime() {
  [[NSUserDefaults standardUserDefaults]
      setDouble:base::Time::Now().InSecondsFSinceUnixEpoch()
         forKey:kLastVariationsSeedFetchTimeKey];
}

}  // namespace

@interface IOSChromeVariationsSeedFetcher (Private)
- (std::unique_ptr<variations::SeedResponse>)
    seedResponseForHTTPResponse:(NSHTTPURLResponse*)httpResponse
                           data:(NSData*)data;
@end

@interface BraveVariationsSeedFetcher () <
    IOSChromeVariationsSeedFetcherDelegate>
@property(nonatomic) dispatch_semaphore_t semaphore;
@end

@implementation BraveVariationsSeedFetcher
- (instancetype)init {
  NSArray* arguments =
      brave::vector_to_ns(base::CommandLine::ForCurrentProcess()->argv());
  self = [super initWithArguments:arguments];
  return self;
}

- (void)startSeedFetch {
  CHECK(false) << "This function must never be called\n";
}

- (void)setDelegate:(id<IOSChromeVariationsSeedFetcherDelegate>)delegate {
  CHECK(false) << "This function must never be called\n";
}

- (void)notifyDelegateSeedFetchResult:(BOOL)result {
  __weak BraveVariationsSeedFetcher* weakSelf = self;
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
    [weakSelf variationsSeedFetcherDidCompleteFetchWithSuccess:result];
  });
}

- (std::unique_ptr<variations::SeedResponse>)
    seedResponseForHTTPResponse:(NSHTTPURLResponse*)httpResponse
                           data:(NSData*)data {
  if ([httpResponse valueForHTTPHeaderField:@"IM"]) {
    return [super seedResponseForHTTPResponse:httpResponse data:data];
  }

  // Seed is NOT gzip compressed.
  // None of Brave's Seeds are gzip compressed.
  // For Chromium, they always are.
  // See: https://github.com/brave/devops/issues/13379
  bool is_gzip_compressed = false;

  NSString* signature =
      [httpResponse valueForHTTPHeaderField:@"X-Seed-Signature"];
  NSString* country = [httpResponse valueForHTTPHeaderField:@"X-Country"];

  auto seed = std::make_unique<variations::SeedResponse>();
  if (data) {
    seed->data =
        std::string(reinterpret_cast<const char*>([data bytes]), [data length]);
  }

  seed->signature = base::SysNSStringToUTF8(signature);
  seed->country = base::SysNSStringToUTF8(country);
  seed->date = base::Time::Now();
  seed->is_gzip_compressed = is_gzip_compressed;
  return seed;
}

- (void)fetchSeedSynchronously {
  [self fetchSeedSynchronously:false];
}

- (void)fetchSeedSynchronously:(bool)ignoringLastSeedFetch {
  base::Time lastSeedFetchTime =
      ignoringLastSeedFetch ? base::Time() : GetLastVariationsSeedFetchTime();
  if (lastSeedFetchTime.is_null()) {
    // Save the last fetch time no matter what
    // If the fetching fails, the next launch will NOT re-fetch!
    SaveLastVariationsSeedFetchTime();

    _semaphore = dispatch_semaphore_create(0);
    [super setDelegate:self];
    [super startSeedFetch];

    // Timeout the semaphore
    // Technically the Variations Seed Fetcher actually has a 1.5s fetch time,
    // so this timeout will never be reached in reality.
    // variations/model/ios_chrome_variations_seed_fetcher.mm;l=27
    dispatch_semaphore_wait(
        _semaphore, dispatch_time(DISPATCH_TIME_NOW, 3.0 * NSEC_PER_SEC));
  }
}

- (void)variationsSeedFetcherDidCompleteFetchWithSuccess:(BOOL)success {
  [super setDelegate:nil];
  dispatch_semaphore_signal(_semaphore);
}
@end
