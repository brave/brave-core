/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "Enums.h"

NS_ASSUME_NONNULL_BEGIN

/// A logger that bridges the C++ logging platform to iOS
NS_SWIFT_NAME(RewardsLogger)
@interface BATBraveRewardsLogger : NSObject

/// Configures the logger by setting a callback function which will be called when the ads and rewards needs to log data.
/// onWrite is called when data needs to be logged
/// onFlush is called when data should be flushed from memory to a file (if needed)
+ (void)configureWithLogCallback:(void(^)(BATLogLevel logLevel, int line, NSString *file, NSString *data))onWrite
                       withFlush:(nullable void (^)())onFlush;
@end

NS_ASSUME_NONNULL_END
