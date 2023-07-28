/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef URLSanitizerService_Private_h
#define URLSanitizerService_Private_h

#import <Foundation/Foundation.h>
#include "brave/ios/browser/api/url_sanitizer/url_sanitizer_service.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"

NS_ASSUME_NONNULL_BEGIN

@interface URLSanitizerService (Private)

- (instancetype)initWithURLSanitizerService:(brave::URLSanitizerService*)urlSanitizer;

@end

NS_ASSUME_NONNULL_END

#endif  // URLSanitizerService_Private_h