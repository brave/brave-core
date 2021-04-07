/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATBraveAds.h"

@interface BATBraveAds (Private)

/// Whether or not Brave Ads is enabled
@property (nonatomic, assign, getter=isEnabled) BOOL enabled;

@end
