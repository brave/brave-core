/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(RewardsLogger)
@interface BATBraveRewardsLogger : NSObject
+ (void)configure:(void(^)(BATLogLevel logLevel, int line, NSString *file, NSString *data))onWrite withFlushCallback:(void(^)())flushCallback;
@end

NS_ASSUME_NONNULL_END
