// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>
#import "ledger.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

@interface BATExternalWallet (DictionaryValue)

- (instancetype)initWithDictionaryValue:(NSDictionary *)dictionary;

/// Dictionary value for saving to prefs
- (NSDictionary *)dictionaryValue;

@end

NS_ASSUME_NONNULL_END
