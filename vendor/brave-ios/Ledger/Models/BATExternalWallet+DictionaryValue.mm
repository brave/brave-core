// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "BATExternalWallet+DictionaryValue.h"
#import "ledger.mojom.objc+private.h"

@implementation BATExternalWallet (DictionaryValue)

- (instancetype)initWithDictionaryValue:(NSDictionary *)dictionary
{
  ledger::mojom::ExternalWallet defaultWallet;
  if ((self = [self initWithExternalWallet:defaultWallet])) {
    self.token = dictionary[@"token"] ?: @"";
    self.address = dictionary[@"address"] ?: @"";
    self.status = static_cast<BATWalletStatus>([dictionary[@"status"] integerValue]);
    self.oneTimeString = dictionary[@"one_time_string"] ?: @"";
    self.userName = dictionary[@"user_name"] ?: @"";
    self.transferred = [dictionary[@"transferred"] boolValue];
  }
  return self;
}

- (NSDictionary *)dictionaryValue
{
  return @{
    @"token": self.token,
    @"address": self.address,
    @"status": @(self.status),
    @"one_time_string": self.oneTimeString,
    @"user_name": self.userName,
    @"transferred": @(self.transferred)
  };
}

- (NSString *)description
{
  return [NSString stringWithFormat:@"%@: token: %@, address: %@, status: %ld, ots: %@, username: %@, transferred: %d",
          [super description], self.token, self.address, self.status, self.oneTimeString, self.userName, self.transferred];
}

@end
