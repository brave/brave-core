// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "GRDHousekeepingAPI.h"
#import "NSLogDisabler.h"

@implementation GRDHousekeepingAPI


- (NSMutableURLRequest *)requestWithEndpoint:(NSString *)apiEndpoint andPostRequestData:(NSData *)postRequestDat {
    NSURL *URL = [NSURL URLWithString:[NSString stringWithFormat:@"https://housekeeping.sudosecuritygroup.com%@", apiEndpoint]];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:URL];
    
    [request setValue:@"GRD-Brave-Build" forHTTPHeaderField:@"X-Guardian-Build"];
    
    NSString *receiptPathString = [[[NSBundle mainBundle] appStoreReceiptURL] path];
    if ([receiptPathString containsString:@"sandboxReceipt"] || [receiptPathString containsString:@"CoreSimulator"]) {
        NSLog(@"Either local device testing, Simulator testing or TestFlight. Setting Sandbox env");
        [request setValue:@"Sandbox" forHTTPHeaderField:@"X-Guardian-Environment"];
    }
    
    [request setHTTPMethod:@"POST"];
    [request setHTTPBody:postRequestDat];
    
    return request;
}

- (void)verifyReceiptWithCompletion:(void (^)(NSArray * _Nullable, BOOL, NSString * _Nullable))completion {
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:@"https://housekeeping.sudosecuritygroup.com/api/v1.1/verify-receipt"]];
    NSData *receiptData = [NSData dataWithContentsOfURL:[[NSBundle mainBundle] appStoreReceiptURL]];
    if (receiptData == nil) {
        NSLog(@"[DEBUG][validate receipt] receiptData == nil");
        if (completion) {
            completion(nil, NO, @"No App Store receipt data present");
        }
        return;
    }
    
    NSData *postData = [NSJSONSerialization dataWithJSONObject:@{@"receipt-data":[receiptData base64EncodedStringWithOptions:0]} options:0 error:nil];
    [request setHTTPBody:postData];
    [request setHTTPMethod:@"POST"];
    [request setCachePolicy:NSURLRequestReloadIgnoringCacheData];
    [request setValue:@"1" forHTTPHeaderField:@"GRD-Brave-Build"];
    
    NSURLSessionDataTask *task = [[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        if (error != nil) {
            NSLog(@"Failed to retrieve receipt data: %@", error);
            if (completion) completion(nil, NO, @"Failed to retrieve receipt data from server");
            return;
        }
        
        NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
        if (statusCode == 500) {
            NSLog(@"Internal server error. Failed to retrieve any receipt information");
            if (completion) completion(nil, NO, @"Internal server error");
            return;
            
        } else if (statusCode == 400) {
            NSLog(@"Bad request. Failed to retrieve any receipt information");
            if (completion) completion(nil, NO, @"Bad request");
            return;
            
        } else if (statusCode == 204) {
            NSLog(@"Successful request. No active subscription found");
            if (completion) completion(nil, YES, nil);
            return;
            
        } else if (statusCode == 200) {
            NSError *jsonError = nil;
            NSArray *validLineItems = [NSJSONSerialization JSONObjectWithData:data options:0 error:&jsonError];
            if (jsonError != nil) {
                NSLog(@"[verifyReceiptWithCompletion] Failed to read valid line items: %@", jsonError);
                if (completion) completion(nil, YES, [NSString stringWithFormat:@"Failed to decode valid line items: %@", [jsonError localizedDescription]]);
                return;
                
            } else {
                if (completion) completion(validLineItems, YES, nil);
                return;
            }
            
        } else {
            NSLog(@"Unknown error: %ld", statusCode);
            if (completion) completion(nil, NO, [NSString stringWithFormat:@"Unknown error. Status code: %ld", statusCode]);
        }
    }];
    [task resume];
}

- (void)createNewSubscriberCredentialWithValidationMethod:(GRDHousekeepingValidationMethod)validationMethod completion:(void (^)(NSString * _Nullable, BOOL, NSString * _Nullable))completion {
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:@"https://housekeeping.sudosecuritygroup.com/api/v1/subscriber-credential/create"]];
    
    NSMutableDictionary *jsonDict = [[NSMutableDictionary alloc] init];
    if (validationMethod == ValidationMethodAppStoreReceipt) {
        NSData *receiptData = [NSData dataWithContentsOfURL:[[NSBundle mainBundle] appStoreReceiptURL]];
        if (receiptData == nil) {
            NSLog(@"[DEBUG] receiptData == nil");
            if (completion) {
                completion(nil, NO, @"AppStore receipt missing");
            }
            return;
        }
        NSString *appStoreReceipt = [receiptData base64EncodedStringWithOptions:0];
        
        [jsonDict setObject:@"iap-brave" forKey:@"validation-method"];
        [jsonDict setObject:appStoreReceipt forKey:@"app-receipt"];
        
    } else if (validationMethod == ValidationMethodPromoCode) {
        if (self.promoCode == nil || [self.promoCode isEqualToString:@""]) {
            if (completion) completion(nil, NO, @"promo code missing");
            return;
        }
        [jsonDict setObject:@"promo-code" forKey:@"validation-method"];
        [jsonDict setObject:self.promoCode forKey:@"promo-code"];
        
    } else if (validationMethod == ValidationMethodFreeUser) {
        [jsonDict setObject:@"free-servers" forKey:@"validation-method"];
        
    } else {
        if (completion) completion(nil, NO, @"validation method missing");
        return;
    }
    
    [request setHTTPMethod:@"POST"];
    [request setHTTPBody:[NSJSONSerialization dataWithJSONObject:jsonDict options:0 error:nil]];
    
    NSURLSessionDataTask *task = [[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        if (error != nil) {
            NSLog(@"Failed to create subscriber credential: %@", [error localizedDescription]);
            if (completion) completion(nil, NO, @"Couldn't create subscriber credential");
            return;
        }
        
        NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
        if (statusCode == 500) {
            NSLog(@"Housekeeping failed to return subscriber credential");
            if (completion) completion(nil, NO, @"Internal server error - couldn't create subscriber credential");
            return;
            
        } else if (statusCode == 400) {
            NSLog(@"Failed to create subscriber credential. Faulty input values");
            if (completion) completion(nil, NO, @"Failed to create subscriber credential. Faulty input values");
            return;
            
        } else if (statusCode == 401) {
            NSLog(@"No subscription present");
            if (completion) completion(nil, NO, @"No subscription present");
            return;
            
        } else if (statusCode == 410) {
            NSLog(@"Subscription expired");
            // Not sending an error message back so that we're not showing a useless error to the user
            // The app should transition to free/unpaid if required
            if (completion) completion(nil, NO, nil);
            return;
            
        } else if (statusCode == 200) {
            NSDictionary *dictFromJSON = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            if (completion) completion([dictFromJSON objectForKey:@"subscriber-credential"], YES, nil);
            return;
            
        } else {
            NSLog(@"Unknown server error");
            if (completion) completion(nil, NO, [NSString stringWithFormat:@"Unknown server error: %ld", statusCode]);
        }
    }];
    [task resume];
}

- (void)requestInvitationWithUUID:(NSString *)inviteUUID completion:(void (^)(GRDGatewayAPIResponse *apiResponse))completion {
    [[DCDevice currentDevice] generateTokenWithCompletionHandler:^(NSData *token, NSError *error) {
        if (error != nil) {
            NSLog(@"[requestInvitaitonWithUUID] Failed to generate Device Check Token: %@", error);
            GRDGatewayAPIResponse *response = [[GRDGatewayAPIResponse alloc] init];
            response.responseStatus = GRDGatewayAPIDeviceCheckError;
            if (completion) completion(response);
            return;
        }
        
        NSString *encodedDeviceToken = [[token base64EncodedStringWithOptions:0] stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet alphanumericCharacterSet]];
        
        NSData *postData = [NSJSONSerialization dataWithJSONObject:@{@"uuid":inviteUUID, @"device-token":encodedDeviceToken} options:0 error:nil];
        
        NSMutableURLRequest *request = [self requestWithEndpoint:@"/api/v1/request-invitation" andPostRequestData:postData];
        
        NSURLSession *session = [NSURLSession sharedSession];
        NSURLSessionDataTask *task = [session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
            GRDGatewayAPIResponse *respObj = [[GRDGatewayAPIResponse alloc] init];
            respObj.urlResponse = response;
                                                    
            if (error) {
                NSLog(@"[DEBUG][requestInvitation] request error = %@", error);
                respObj.error = error;
                respObj.responseStatus = GRDGatewayAPIUnknownError;
                if (completion) completion(respObj);
            } else {
                if ([(NSHTTPURLResponse *)response statusCode] == 200) {
                    NSLog(@"[DEBUG][requestInvitation] success!");
                    respObj.responseStatus = GRDGatewayAPISuccess;
                    
                } else if ([(NSHTTPURLResponse *)response statusCode] == 403) {
                    NSLog(@"[DEBUG][requestInvitation] DeviceCheck validation failed!");
                    respObj.responseStatus = GRDGatewayAPIDeviceCheckError;
                    
                } else if ([(NSHTTPURLResponse *)response statusCode] == 404) {
                    NSLog(@"[DEBUG][requestInvitation] API endpoint not found!");
                    respObj.responseStatus = GRDGatewayAPIEndpointNotFound;
                    
                } else if ([(NSHTTPURLResponse *)response statusCode] == 500) {
                    NSLog(@"[DEBUG][requestInvitation] Server error! Need to use different server");
                    respObj.responseStatus = GRDGatewayAPIServerInternalError;
                    
                } else {
                    NSLog(@"[DEBUG][requestInvitation] unknown error!");
                    respObj.responseStatus = GRDGatewayAPIUnknownError;
                }
                                                        
                if (data != nil) {
                    // shouldn't get to here as is, but we may change to return error code via JSON
                    NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
                    respObj.jsonData = json;
                }
                                                        
                if (completion) completion(respObj);
            }
        }];
        [task resume];
    }];
}

- (void)addPushToken:(NSString *)pushToken toInvitationUUID:(NSString *)inviteUUID completion:(void (^)(GRDGatewayAPIResponse *apiResponse))completion {
    [[DCDevice currentDevice] generateTokenWithCompletionHandler:^(NSData *token, NSError *error) {
        if (error != nil) {
            NSLog(@"[addPshToken:toInvitationUUID] Failed to generate Device Check Token: %@", error);
            GRDGatewayAPIResponse *response = [[GRDGatewayAPIResponse alloc] init];
            response.responseStatus = GRDGatewayAPIDeviceCheckError;
            if (completion) completion(response);
            return;
        }
        
        NSString *encodedDeviceToken = [[token base64EncodedStringWithOptions:0] stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet alphanumericCharacterSet]];
        
        NSData *postData = [NSJSONSerialization dataWithJSONObject:@{@"uuid":inviteUUID, @"push-token":pushToken, @"device-token":encodedDeviceToken} options:0 error:nil];
        
        NSMutableURLRequest *request = [self requestWithEndpoint:@"/api/v1/add-push-token-to-invitation" andPostRequestData:postData];
        
        NSURLSession *session = [NSURLSession sharedSession];
        NSURLSessionDataTask *task = [session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
            GRDGatewayAPIResponse *respObj = [[GRDGatewayAPIResponse alloc] init];
            respObj.urlResponse = response;
                                                    
            if (error) {
                NSLog(@"[DEBUG][requestInvitation] request error = %@", error);
                respObj.error = error;
                respObj.responseStatus = GRDGatewayAPIUnknownError;
                if (completion) completion(respObj);
            } else {
                if ([(NSHTTPURLResponse *)response statusCode] == 200) {
                    NSLog(@"[DEBUG][addPushToInvitationUUID] success!");
                    respObj.responseStatus = GRDGatewayAPISuccess;
                    
                } else if ([(NSHTTPURLResponse *)response statusCode] == 403) {
                    NSLog(@"[DEBUG][addPushToInvitationUUID] DeviceCheck validation failed!");
                    respObj.responseStatus = GRDGatewayAPIDeviceCheckError;
                    
                } else if ([(NSHTTPURLResponse *)response statusCode] == 404) {
                    NSLog(@"[DEBUG][addPushToInvitationUUID] API endpoint not found!");
                    respObj.responseStatus = GRDGatewayAPIEndpointNotFound;
                    
                } else if ([(NSHTTPURLResponse *)response statusCode] == 500) {
                    NSLog(@"[DEBUG][addPushToInvitationUUID] Server error! Need to use different server");
                    respObj.responseStatus = GRDGatewayAPIServerInternalError;
                    
                } else {
                    NSLog(@"[DEBUG][addPushToInvitationUUID] unknown error!");
                    respObj.responseStatus = GRDGatewayAPIUnknownError;
                }
                                                        
                if (data != nil) {
                    // shouldn't get to here as is, but we may change to return error code via JSON
                    NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
                    respObj.jsonData = json;
                }
                                                        
                if (completion) completion(respObj);
            }
        }];
        [task resume];
    }];
}


#pragma mark - Time Zone & VPN Hostname endpoints

- (void)requestTimeZonesForRegionsWithTimestamp:(NSNumber *)timestamp completion:(void (^)(NSArray *timezones, BOOL success, NSUInteger responseStatusCode))completion {
    NSData *requestJSON = [NSJSONSerialization dataWithJSONObject:@{@"timestamp":timestamp} options:0 error:nil];
    NSMutableURLRequest *request = [self requestWithEndpoint:@"/api/v1/servers/timezones-for-regions" andPostRequestData:requestJSON];
    [request setCachePolicy:NSURLRequestReloadIgnoringLocalCacheData];
    
    NSURLSession *session = [NSURLSession sharedSession];
    NSURLSessionDataTask *task = [session dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        
        if (error != nil) {
            NSLog(@"[requestTimeZonesForRegionsWithTimestamp] Failed to hit endpoint: %@", error);
            if (completion) completion(nil, NO, 0);
            return;
        }
        
        NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
        if (statusCode == 500) {
            NSLog(@"[requestTimeZonesForRegionsWithTimestamp] Internal server error");
            if (completion) completion(nil, NO, statusCode);
            
        } else if (statusCode == 304) {
            if (completion) completion(nil, YES, statusCode);
            
        } else if (statusCode == 200) {
            NSArray *timezones = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            if (completion) completion(timezones, YES, statusCode);
            
        } else {
            NSLog(@"[requestTimeZonesForRegionsWithTimestamp] Uncaught http response status: %ld", statusCode);
            if (completion) completion(nil, NO, statusCode);
            return;
        }
    }];
    [task resume];
}

- (void)requestServersForRegion:(NSString *)region completion:(void (^)(NSArray *, BOOL))completion {
    NSNumber *payingUserAsNumber = [NSNumber numberWithBool:[GRDVPNHelper isPayingUser]];
    NSData *requestJSON = [NSJSONSerialization dataWithJSONObject:@{@"region":region, @"paid":payingUserAsNumber} options:0 error:nil];
    NSMutableURLRequest *request = [self requestWithEndpoint:@"/api/v1/servers/hostnames-for-region" andPostRequestData:requestJSON];
    [request setCachePolicy:NSURLRequestReloadIgnoringLocalCacheData];
    
    NSURLSession *session = [NSURLSession sharedSession];
    NSURLSessionDataTask *task = [session dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        
        if (error != nil) {
            NSLog(@"[requestServersForRegion] Failed to hit endpoint: %@", error);
            if (completion) completion(nil, NO);
            return;
        }
        
        NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
        if (statusCode == 400) {
            NSLog(@"[requestServersForRegion] region key missing or mangled in JSON");
            if (completion) completion(nil, NO);
            return;
            
        } else if (statusCode == 500) {
            NSLog(@"[requestServersForRegion] Internal server error");
            if (completion) completion(nil, NO);
            return;
            
        } else if (statusCode == 200) {
            NSArray *servers = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            if (completion) {
                completion(servers, YES);
            }
        } else {
            NSLog(@"[requestServersForRegion] Uncaught http response status: %ld", statusCode);
            if (completion) completion(nil, NO);
            return;
        }
    }];
    [task resume];
}

- (void)requestAllHostnamesWithCompletion:(void (^)(NSArray * _Nullable, BOOL))completion {
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:@"https://housekeeping.sudosecuritygroup.com/api/v1/servers/all-hostnames"]];
    [request setCachePolicy:NSURLRequestReloadIgnoringCacheData];
    
    NSURLSessionDataTask *task = [[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        if (error != nil) {
            NSLog(@"[requestAllHostnamesWithCompletion] Request failed: %@", error);
            if (completion) completion(nil, NO);
            return;
        }
        
        NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
        if (statusCode == 500) {
            NSLog(@"[requestAllHostnamesWithCompletion] Internal server error");
            if (completion) completion(nil, NO);
            
        } else if (statusCode == 200) {
            NSArray *servers = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            if (completion) completion(servers, YES);
            
        } else {
            NSLog(@"[requestAllHostnamesWithCompletion] Uncaught http response status: %ld", statusCode);
            if (completion) completion(nil, NO);
            return;
        }
    }];
    [task resume];
}

@end
