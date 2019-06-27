/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "ledger.mojom.objc.h"
#import "bat/ledger/publisher_info.h"
#import "bat/ledger/pending_contribution.h"

NS_ASSUME_NONNULL_BEGIN

@interface BATPublisherInfo (PrivateCpp)

- (ledger::PublisherInfoPtr)cppObj;

@end

@interface BATPendingContributionInfo (PrivateCpp)
- (ledger::PendingContributionInfoPtr)cppObj;
@end

NS_ASSUME_NONNULL_END
