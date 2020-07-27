// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "GRDSubscriberCredential.h"
#import "NSLogDisabler.h"

@implementation GRDSubscriberCredential

- (instancetype)initWithSubscriberCredential:(NSString *)subscriberCredential {
    if (!self) {
        self = [super init];
    }
    
    self.subscriberCredential = subscriberCredential;
    [self processSubscriberCredentialInformation];
    
    return self;
}

- (void)processSubscriberCredentialInformation {
    if (self.subscriberCredential == nil) {
        return;
    }
    
    NSArray *jwtComp = [self.subscriberCredential componentsSeparatedByString:@"."];
    NSString *payloadString = [jwtComp objectAtIndex:1];
    
    // Note from CJ:
    // This is Base64 magic that I only partly understand because I am not entirely familiar with
    // the Base64 spec.
    // This just makes sure that the string can be read by removing invalid characters
    payloadString = [[payloadString stringByReplacingOccurrencesOfString:@"-" withString:@"+"] stringByReplacingOccurrencesOfString:@"_" withString:@"/"];
    
    // Figuring out how many buffer characters we're missing
    int size = [payloadString length] % 4;
    
    // Creating a mutable string from the payloadString
    NSMutableString *base64String = [[NSMutableString alloc] initWithString:payloadString];
    
    // Adding as many buffer = as required to make the payloadString divisble by 4 to make
    // it Base64 spec compliant so that NSData will accept it and decode it
    for (int i = 0; i < size; i++) {
        [base64String appendString:@"="];
    }
    
    NSData *payload = [[NSData alloc] initWithBase64EncodedString:base64String options:0];
    NSDictionary *dict = [NSJSONSerialization JSONObjectWithData:payload options:0 error:nil];
    self.subscriptionType = [dict objectForKey:@"subscription-type"];
    self.subscriptionTypePretty = [dict objectForKey:@"subscription-type-pretty"];
    self.subscriptionExpirationDate = [(NSNumber*)[dict objectForKey:@"subscription-expiration-date"] integerValue];
    self.tokenExpirationDate = [(NSNumber*)[dict objectForKey:@"exp"] integerValue];
    
    NSInteger nowUnix = [[NSDate date] timeIntervalSince1970];
    if (nowUnix >= self.tokenExpirationDate) {
        self.tokenExpired = YES;
    } else {
        self.tokenExpired = NO;
    }
}

@end
