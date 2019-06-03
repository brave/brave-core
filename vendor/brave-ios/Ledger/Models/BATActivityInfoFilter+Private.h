/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATActivityInfoFilter.h"
#import "bat/ledger/publisher_info.h"

@interface BATActivityInfoFilterOrderPair (Private)
- (instancetype)initWithStringBoolPair:(std::pair<std::string, bool>)obj;
- (std::pair<std::string, bool>)cppObj;
@end

@interface BATActivityInfoFilter (Private)
- (instancetype)initWithActivityInfoFilter:(const ledger::ActivityInfoFilter&)obj;
- (ledger::ActivityInfoFilter)cppObj;
@end
