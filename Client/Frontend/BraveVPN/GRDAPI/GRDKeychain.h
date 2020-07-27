// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

#define kKeychainStr_EapUsername @"eap-username"
#define kKeychainStr_EapPassword @"eap-password"
#define kKeychainStr_AuthToken @"auth-token"
#define kKeychainStr_APIAuthToken @"api-auth-token"
#define kKeychainStr_SubscriberCredential @"subscriber-credential"

@interface GRDKeychain : NSObject

+ (OSStatus)storePassword:(NSString *)passwordStr forAccount:(NSString *)accountKeyStr retry:(BOOL)retry;
+ (NSString *)getPasswordStringForAccount:(NSString *)accountKeyStr;
+ (NSData *)getPasswordRefForAccount:(NSString *)accountKeyStr;
+ (OSStatus)removeKeychanItemForAccount:(NSString *)accountKeyStr;
+ (void)removeGuardianKeychainItems;

@end
