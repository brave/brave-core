/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPARKLE_GLUE_MAC_H_
#define BRAVE_BROWSER_SPARKLE_GLUE_MAC_H_

#include <string>

#if defined(__OBJC__)

#import <Foundation/Foundation.h>

@interface SparkleGlue : NSObject

+ (instancetype)sharedSparkleGlue;

+ (std::string)descriptionFromAppcastItem:(id)item;

- (instancetype)init;

- (void)setDelegate:(id)delegate;
- (void)checkForUpdates:(id)sender;

@end  // @interface SparkleGlue

#endif  // __OBJC__

#endif  // BRAVE_BROWSER_SPARKLE_GLUE_MAC_H_
