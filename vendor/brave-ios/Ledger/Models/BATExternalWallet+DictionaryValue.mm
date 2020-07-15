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
    self.anonAddress = dictionary[@"anon_address"] ?: @"";
    self.status = static_cast<BATWalletStatus>([dictionary[@"status"] integerValue]);
    self.oneTimeString = dictionary[@"one_time_string"] ?: @"";
    self.userName = dictionary[@"user_name"] ?: @"";
    self.verifyUrl = dictionary[@"verify_url"] ?: @"";
    self.addUrl = dictionary[@"add_url"] ?: @"";
    self.withdrawUrl = dictionary[@"withdraw_url"] ?: @"";
    self.accountUrl = dictionary[@"account_url"] ?: @"";
  }
  return self;
}

- (NSDictionary *)dictionaryValue
{
  return @{
    @"token": self.token,
    @"address": self.address,
    @"anon_address": self.anonAddress,
    @"status": @(self.status),
    @"one_time_string": self.oneTimeString,
    @"user_name": self.userName,
    @"verify_url": self.verifyUrl,
    @"add_url": self.addUrl,
    @"withdraw_url": self.withdrawUrl,
    @"account_url": self.accountUrl
  };
}

- (NSString *)description
{
  return [NSString stringWithFormat:@"%@: token: %@, address: %@,  anonAddress: %@, status: %ld, ots: %@, username: %@, verifyUrl: %@, addUrl: %@, withdrawUrl: %@, accountUrl: %@",
          [super description], self.token, self.address, self.anonAddress, self.status, self.oneTimeString, self.userName, self.verifyUrl, self.addUrl, self.withdrawUrl, self.accountUrl];
}

@end
