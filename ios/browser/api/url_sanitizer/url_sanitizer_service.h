/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef URLSanitizerService_h
#define URLSanitizerService_h

#import <Foundation/Foundation.h>

@interface URLSanitizerService : NSObject
- (instancetype)init NS_UNAVAILABLE;

/**
 Sanitizes the given URL.
 
 @param url The URL to be sanitized.
 @return A sanitized NSURL object.
 */
- (NSURL *)sanitizedURL:(NSURL *)url;

@end

#endif /* URLSanitizerService_h */