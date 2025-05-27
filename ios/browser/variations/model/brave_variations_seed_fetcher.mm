/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/variations/model/brave_variations_seed_fetcher.h"

#include "base/command_line.h"
#include "brave/base/mac/conversions.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/variations/model/ios_chrome_variations_seed_fetcher.h"

@interface BraveVariationsSeedFetcher () <
    IOSChromeVariationsSeedFetcherDelegate>
@property(nonatomic, strong) IOSChromeVariationsSeedFetcher* fetcher;
@property(nonatomic, strong) void (^completion)(bool);
@end

@implementation BraveVariationsSeedFetcher
- (instancetype)init {
  if ((self = [super init])) {
    NSArray* arguments =
        brave::vector_to_ns(base::CommandLine::ForCurrentProcess()->argv());

    CHECK(arguments);

    _fetcher =
        [[IOSChromeVariationsSeedFetcher alloc] initWithArguments:arguments];
    CHECK(_fetcher);
  }
  return self;
}

- (void)fetchSeedWithCompletion:(void (^)(bool success))completion {
  self.completion = completion;
  _fetcher.delegate = self;
  [_fetcher startSeedFetch];
}

- (void)variationsSeedFetcherDidCompleteFetchWithSuccess:(BOOL)success {
  if (_completion) {
    _completion(success);
  }
}
@end
