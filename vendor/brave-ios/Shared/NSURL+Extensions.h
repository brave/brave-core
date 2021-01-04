/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface NSURL (Extensions)

@property (nonatomic, nullable, readonly) NSString *bat_normalizedHost;

@end

NS_ASSUME_NONNULL_END
