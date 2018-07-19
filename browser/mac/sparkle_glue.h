/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MAC_SPARKLE_GLUE_H_
#define BRAVE_BROWSER_MAC_SPARKLE_GLUE_H_

#include <string>

#if defined(__OBJC__)

#import <Foundation/Foundation.h>

@interface SparkleGlue : NSObject

+ (instancetype)sharedSparkleGlue;

- (instancetype)init;

- (void)registerWithSparkle;

- (void)checkForUpdates;
- (void)checkForUpdatesInBackground;

// Returns YES if the application is running from a read-only filesystem,
// such as a disk image.
- (BOOL)isOnReadOnlyFilesystem;

@end  // @interface SparkleGlue

#endif  // __OBJC__

#endif  // BRAVE_BROWSER_MAC_SPARKLE_GLUE_H_
