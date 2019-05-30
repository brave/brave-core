/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATPublisherInfo.h"
#import "bat/ledger/publisher_info.h"

NS_ASSUME_NONNULL_BEGIN

@interface BATPublisherInfo (Private)

- (instancetype)initWithPublisherInfo:(const ledger::PublisherInfo&)o;

@end

@interface BATPublisherInfo (PrivateCpp)

- (ledger::PublisherInfoPtr)cppObj;

@end

NS_ASSUME_NONNULL_END
