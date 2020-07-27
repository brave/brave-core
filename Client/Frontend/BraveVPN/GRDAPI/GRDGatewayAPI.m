// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "GRDGatewayAPI.h"
#import <NetworkExtension/NetworkExtension.h>
#import "VPNConstants.h"
#import "NSLogDisabler.h"

@implementation GRDGatewayAPI
@synthesize apiAuthToken, deviceIdentifier;
@synthesize apiHostname, healthCheckTimer;

+ (instancetype)sharedAPI {
    static GRDGatewayAPI *sharedAPI = nil;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        sharedAPI = [[self alloc] init];
    });
    return sharedAPI;
}

- (BOOL)isVPNConnected {
    return ([[[NEVPNManager sharedManager] connection] status] == NEVPNStatusConnected);
}

- (void)stopHealthCheckTimer {
    if (self.healthCheckTimer != nil) {
        [self.healthCheckTimer invalidate];
        self.healthCheckTimer = nil;
    }
}

- (void)startHealthCheckTimer {
    [self stopHealthCheckTimer];
    self.healthCheckTimer = [NSTimer scheduledTimerWithTimeInterval:10 repeats:true block:^(NSTimer * _Nonnull timer) {
        [self networkHealthCheck];
    }];
}

- (void)networkHealthCheck {
    [self networkProbeWithCompletion:^(BOOL status, NSError *error) {
        GRDNetworkHealthType health = GRDNetworkHealthUnknown;
        if ([error code] == NSURLErrorNotConnectedToInternet ||
            //[error code] == NSURLErrorTimedOut || // comment out until we are 100% file will be available - network health NEVER comes back as bad when this is off during testing.
            [error code] == NSURLErrorInternationalRoamingOff || [error code] == NSURLErrorDataNotAllowed) {
            health = GRDNetworkHealthBad;
        } else {
            health = GRDNetworkHealthGood;
        }
        
        // BRAVE TODO: Add notification for it?
//        [[NSNotificationCenter defaultCenter] postNotificationName:kGuardianNetworkHealthStatusNotification object:[NSNumber numberWithInteger:health]];
    }];
}

- (void)_loadCredentialsFromKeychain {
    apiAuthToken = [GRDKeychain getPasswordStringForAccount:kKeychainStr_APIAuthToken];
    deviceIdentifier = [GRDKeychain getPasswordStringForAccount:kKeychainStr_EapUsername];
    
    if (apiAuthToken != nil) {
        NSLog(@"[DEBUG][loadCredentials] we have authToken (%@)", apiAuthToken);
    } else {
        NSLog(@"[DEBUG][loadCredentials] no authToken !!!");
    }
    
    if (deviceIdentifier != nil) {
        NSLog(@"[DEBUG][loadCredentials] we have deviceIdentifier (%@)", deviceIdentifier);
    } else {
        NSLog(@"[DEBUG][loadCredentials] no deviceIdentifier !!!");
    }
}

- (NSString *)_baseHostname {
    if (apiHostname == nil) {
        NSLog(@"[DEBUG][GRDGatewayAPI][_baseHostname] apiHostname==nil, loading the APIHostname-Override");
        
        // this should be removed some time when we deem that it 100% will not break shit if we do
        // FYI - I am keeping this as the direct user defaults read, because this API object should never interact with VPN helper object
        if ([[NSUserDefaults standardUserDefaults] objectForKey:kGRDHostnameOverride]) {
            apiHostname = [[NSUserDefaults standardUserDefaults] objectForKey:kGRDHostnameOverride];
            NSLog(@"[DEBUG][GRDGatewayAPI][_baseHostname] Override for API present, setting base hostname to: %@", apiHostname);
            return apiHostname;
        } else {
            return nil;
        }
    } else {
        return apiHostname;
   }
}

- (BOOL)_canMakeApiRequests {
    if ([self _baseHostname] == nil) {
        return NO;
    } else {
        return YES;
    }
}

- (NSMutableURLRequest *)_requestWithEndpoint:(NSString *)apiEndpoint andPostRequestString:(NSString *)postRequestStr {
    NSURL *URL = [NSURL URLWithString:[NSString stringWithFormat:@"https://%@%@", [self _baseHostname], apiEndpoint]];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:URL];
    
    [request setHTTPMethod:@"POST"];
    [request setHTTPBody:[postRequestStr dataUsingEncoding:NSUTF8StringEncoding]];
    
    return request;
}

- (NSMutableURLRequest *)_requestWithEndpoint:(NSString *)apiEndpoint andPostRequestData:(NSData *)postRequestDat {
    NSURL *URL = [NSURL URLWithString:[NSString stringWithFormat:@"https://%@%@", [self _baseHostname], apiEndpoint]];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:URL];
	
	[request setValue:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"] forHTTPHeaderField:@"X-Guardian-Build"];
    
	NSString *receiptPathString = [[[NSBundle mainBundle] appStoreReceiptURL] path];
	if ([receiptPathString containsString:@"sandboxReceipt"] || [receiptPathString containsString:@"CoreSimulator"]) {
		NSLog(@"Either local device testing, Simulator testing or TestFlight. Setting Sandbox env");
		[request setValue:@"Sandbox" forHTTPHeaderField:@"X-Guardian-Environment"];
	}
	
	[request setHTTPMethod:@"POST"];
    [request setHTTPBody:postRequestDat];
    
    return request;
}

+ (GRDGatewayAPIResponse *)deniedResponse {
    GRDGatewayAPIResponse *response = [[GRDGatewayAPIResponse alloc] init];
    response.responseStatus = GRDGatewayAPIStatusAPIRequestsDenied;
    return response;
}

+ (GRDGatewayAPIResponse *)missingTokenResponse {
    GRDGatewayAPIResponse *response = [[GRDGatewayAPIResponse alloc] init];
    response.responseStatus = GRDGatewayAPITokenMissing;
    return response;
}

- (void)getServerStatusWithCompletion:(void (^)(GRDGatewayAPIResponse *apiResponse))completion {
    if ([self _canMakeApiRequests] == NO) {
        NSLog(@"[DEBUG][getServerStatus] cannot make API requests !!! won't continue");
        if (completion) {
            completion([GRDGatewayAPI deniedResponse]);
        }
        return;
    }
    
    NSURL *URL = [NSURL URLWithString:[NSString stringWithFormat:@"https://%@%@", [self _baseHostname], kSGAPI_ServerStatus]];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:URL];
    [request setTimeoutInterval:10.0f];
    [request setHTTPMethod:@"GET"];
    [request setValue:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"] forHTTPHeaderField:@"X-Guardian-Build"];
    
    NSURLSession *session = [NSURLSession sharedSession];
    NSURLSessionDataTask *task = [session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        GRDGatewayAPIResponse *respObj = [[GRDGatewayAPIResponse alloc] init];
        respObj.urlResponse = response;
        
        if (error) {
            if (error.code == NSURLErrorCannotConnectToHost) {
                NSLog(@"Couldn't get server status. Host is offline");
                respObj.responseStatus = GRDGatewayAPIServerNotOK;
                if (completion) completion(respObj);
                
            } else {
                NSLog(@"[DEBUG][getServerStatus] request error = %@", error);
                respObj.error = error;
                respObj.responseStatus = GRDGatewayAPIUnknownError;
                completion(respObj);
            }
        } else {
            if ([(NSHTTPURLResponse *)response statusCode] == 200) {
                respObj.responseStatus = GRDGatewayAPIServerOK;
            } else if ([(NSHTTPURLResponse *)response statusCode] == 500) {
                NSLog(@"[DEBUG][getServerStatus] Server error! Need to use different server");
                respObj.responseStatus = GRDGatewayAPIServerInternalError;
            } else if ([(NSHTTPURLResponse *)response statusCode] == 404) {
                NSLog(@"[DEBUG][getServerStatus] Endpoint not found on this server!");
                respObj.responseStatus = GRDGatewayAPIEndpointNotFound;
            } else {
                NSLog(@"[DEBUG][getServerStatus] unknown error!");
                respObj.responseStatus = GRDGatewayAPIUnknownError;
            }
            
            if (data != nil) {
                // The /server-status endpoint has been changed to never return data.
                // This is legacy that way meant to report the capcity-score to the app
                // It is now reported through housekeeping. This will be removed in the next API iteration
                NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
                respObj.jsonData = json;
            }
            completion(respObj);
        }
    }];
    
    [task resume];
}


- (void)registerAndCreateWithSubscriberCredential:(NSString *)subscriberCredential completion:(void (^)(NSDictionary * _Nullable, BOOL, NSString * _Nullable))completion {
    if ([self _canMakeApiRequests] == NO) {
        NSLog(@"[DEBUG][registerAndCreateWithSubscriberCredential] cannot make API requests !!! won't continue");
        if (completion) {
            completion(nil, NO, @"No VPN server selected. Can't create EAP Crendentials");
        }
        return;
    }
    
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithFormat:@"https://%@/api/v1.1/register-and-create", [self _baseHostname]]]];
    [request setHTTPMethod:@"POST"];
    
    NSDictionary *jsonDict = @{@"subscriber-credential":subscriberCredential};
    [request setHTTPBody:[NSJSONSerialization dataWithJSONObject:jsonDict options:0 error:nil]];
    
    NSURLSessionDataTask *task = [[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        if (error != nil) {
            NSLog(@"Couldn't connect to host: %@", [error localizedDescription]);
            if (completion) completion(nil, NO, @"Error authenticating with subscriber credential");
            return;
        }
        
        NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
        if (statusCode == 500) {
            NSLog(@"Internal server error authenticating with subscriber credential");
            if (completion) completion(nil, NO, @"Internal server error authenticating with subscriber credential");
            return;
            
        } else if (statusCode == 410) {
            NSLog(@"Subscriber credential invalid");
            if (completion) completion(nil, NO, @"Subscriber credential invalid");
            return;
            
        } else if (statusCode == 400) {
            NSLog(@"Subscriber credential missing");
            if (completion) completion(nil, NO, @"Subscriber credential missing");
            return;
            
        } else if (statusCode == 402) {
            NSLog(@"Free user trying to connect to a paid only server");
            if (completion) completion(nil, NO, @"Trying to connect to a premium server as a free user");
            return;
        } else if (statusCode == 200) {
            NSDictionary *dictFromJSON = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            if (completion) completion(dictFromJSON, YES, nil);
            return;
            
        } else {
            NSLog(@"Unknown error: %ld", statusCode);
            if (completion) completion(nil, NO, [NSString stringWithFormat:@"Unknown error: %ld", statusCode]);
        }
    }];
    [task resume];
}

//unused

// full (prototype) endpoint: "/vpnsrv/api/device/<device_token>/set-push-token"
// input: "auth-token" and "push-token" (POST format)
- (void)bindPushToken:(NSString *)pushTok notificationMode:(NSString *)notifMode {
    if ([self _canMakeApiRequests] == NO) {
        NSLog(@"[DEBUG][bindPushToken] cannot make API requests !!! won't continue");
        return;
    }
    
    if (apiAuthToken == nil ) {
        NSLog(@"[DEBUG][bindAPNs] no auth token! cannot bind push token.");
        return;
    } else if (deviceIdentifier == nil) {
        NSLog(@"[DEBUG][bindAPNs] no device id! cannot bind push token.");
        return;
    }
    
    NSString *notifModeValue = nil;
    if ([notifMode isEqualToString:@"instant"]) {
        NSLog(@"[DEBUG][bindAPNs] notification mode is instant");
        notifModeValue = @"1";
    } else if ([notifMode isEqualToString:@"daily"]) {
        NSLog(@"[DEBUG][bindAPNs] notification mode is daily");
        notifModeValue = @"2";
    } else {
        NSLog(@"[DEBUG][bindAPNs] notification mode not recognized (%@), defaulting to daily...", notifMode);
        notifModeValue = @"2";
    }
    
    NSString *endpointStr = [NSString stringWithFormat:@"%@/%@%@", kSGAPI_DeviceBase, deviceIdentifier, kSGAPI_Device_SetPushToken];
    NSString *postDataStr = [NSString stringWithFormat:@"auth-token=%@&push-token=%@&notification-mode=%@", apiAuthToken, pushTok, notifModeValue];
    NSURLRequest *request = [self _requestWithEndpoint:endpointStr andPostRequestString:postDataStr];
    
    // TO DO
    // replace response data with GRDGatewayAPIResponse object
    NSURLSession *session = [NSURLSession sharedSession];
    NSURLSessionDataTask *task = [session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        if (error) {
            NSLog(@"[DEBUG][bindAPNs] request error = %@", error);
        } else {
            if (data == nil) {
                NSLog(@"[DEBUG][bindAPNs] data == nil");
                return; // FIXME
            }
            
            NSDictionary *json  = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            NSLog(@"[DEBUG][bindAPNs] json response data = %@",json);
        }
    }];
    
    [task resume];
}

- (NSArray *)_fakeAlertsArray {
    NSString *curDateStr = [NSString stringWithFormat:@"%f", [[NSDate date] timeIntervalSince1970]];
    NSMutableArray *fakeAlerts = [NSMutableArray array];
   
    NSInteger i = 0;
    for (i = 0; i < 20000; i++){
        [fakeAlerts addObject:@{@"action":@"drop",
                                @"category":@"privacy-tracker-app",
                                @"host":@"analytics.localytics.com",
                                @"message":@"Prevented 'Localytics' from obtaining unknown data from device. Prevented 'Localytics' from obtaining unknown data from device Prevented 'Localytics' from obtaining unknown data from device Prevented 'Localytics' from obtaining unknown",
                                @"timestamp":curDateStr,
                                @"title":@"Blocked Data Tracker",
                                @"uuid":[[NSUUID UUID] UUIDString] }];
        
        [fakeAlerts addObject:@{@"action":@"drop",
                                @"category":@"privacy-tracker-app-location",
                                @"host":@"api.beaconsinspace.com",
                                @"message":@"Prevented 'Beacons In Space' from obtaining unknown data from device",
                                @"timestamp":curDateStr,
                                @"title":@"Blocked Location Tracker",
                                @"uuid":[[NSUUID UUID] UUIDString] }];
        
        [fakeAlerts addObject:@{@"action":@"drop",
                                @"category":@"security-phishing",
                                @"host":@"api.phishy-mcphishface-thisisanexampleofalonghostname.com",
                                @"message":@"Prevented 'Phishy McPhishface' from obtaining unknown data from device",
                                @"timestamp":curDateStr,
                                @"title":@"Blocked Phishing Attempt",
                                @"uuid":[[NSUUID UUID] UUIDString] }];
        
        [fakeAlerts addObject:@{@"action":@"drop",
                                @"category":@"encryption-allows-invalid-https",
                                @"host":@"facebook.com",
                                @"message":@"Prevented 'Facebook', you're welcome",
                                @"timestamp":curDateStr,
                                @"title":@"Blocked MITM",
                                @"uuid":[[NSUUID UUID] UUIDString] }];
        
        [fakeAlerts addObject:@{@"action":@"drop",
                                @"category":@"ads/aggressive",
                                @"host":@"google.com",
                                @"message":@"Prevented Google from forcing shit you don't need down your throat",
                                @"timestamp":curDateStr,
                                @"title":@"Blocked Ad Tracker",
                                @"uuid":[[NSUUID UUID] UUIDString] }];
    }
    
    
    
    return [NSArray arrayWithArray:fakeAlerts];
}

- (void)getEvents:(void(^)(NSDictionary *response, BOOL success, NSString *error))completion {
    if (self.dummyDataForDebugging == NO) {
        if ([self _canMakeApiRequests] == NO) {
            NSLog(@"[DEBUG][getEvents] cannot make API requests !!! won't continue");
            if (completion) completion(nil, NO, @"cant make API requests");
            return;
        }
        
        if (!deviceIdentifier) {
            if (completion) completion(nil, NO, @"An error occured!, Missing device id!");
            return;
        }
        
        NSString *apiEndpoint = [NSString stringWithFormat:@"/api/v1.1/device/%@/alerts", deviceIdentifier];
        NSString *finalHost = [NSString stringWithFormat:@"https://%@%@", [self _baseHostname], apiEndpoint];
        
        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL: [NSURL URLWithString:finalHost]];
        NSDictionary *jsonDict = @{kKeychainStr_APIAuthToken:apiAuthToken};
        [request setHTTPBody:[NSJSONSerialization dataWithJSONObject:jsonDict options:0 error:nil]];
        [request setHTTPMethod:@"POST"];
        
        NSURLSessionDataTask *task = [[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
            if (error != nil) {
                NSLog(@"Couldn't connect to host: %@", [error localizedDescription]);
                if (completion) completion(nil, NO, @"Error connecting to host for getEvents");
                return;
            }
            
            NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
            if (statusCode == 500) {
                NSLog(@"Internal server error");
                if (completion) completion(nil, NO,@"Internal server error" );
                return;
                
            } else if (statusCode == 401) {
                NSLog(@"Auth failure. Needs to migrate device");
                [[NSUserDefaults standardUserDefaults] setBool:YES forKey:kAppNeedsSelfRepair];
                if (completion) completion(nil, NO, @"Authentication failed. Server migration required");
                return;
                
            } else if (statusCode == 400) {
                NSLog(@"Bad Request");
                if (completion) completion(nil, NO, @"Subscriber credential missing");
                return;
                
            } else if (statusCode == 200) {
                NSError *jsonError = nil;
                NSDictionary *dictFromJSON = [NSJSONSerialization JSONObjectWithData:data options:0 error:&jsonError];
                if (jsonError) {
                    NSLog(@"Failed to decode JSON with alerts: %@", jsonError);
                    if (completion) completion(nil, NO, @"Failed to decode JSON");
                } else {
                    if (completion) completion(dictFromJSON, YES, nil);
                }
                return;
                
            } else {
                NSLog(@"Unknown error: %ld", statusCode);
                if (completion) completion(nil, NO, [NSString stringWithFormat:@"Unknown error: %ld", statusCode]);
            }
        }];
        [task resume];
        
    } else {
        // Returning dummy data so that we can debug easily in the simulator
        completion([NSDictionary dictionaryWithObject:[self _fakeAlertsArray] forKey:@"alerts"], YES, nil);
    }
}


- (void)networkProbeWithCompletion:(void (^)(BOOL status, NSError *error))completion {
    //https://guardianapp.com/network-probe.txt
    //easier than the usual setup, and doing it in the bg so it will be fine.
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        NSURL *URL = [NSURL URLWithString:@"https://guardianapp.com/network-probe.txt"];
        NSMutableURLRequest *request = [[NSMutableURLRequest alloc] initWithURL:URL cachePolicy:NSURLRequestReloadIgnoringCacheData timeoutInterval:5.0];
        
        NSURLSession *session = [NSURLSession sharedSession];
        NSURLSessionDataTask *task = [session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
			if (error) {
				NSLog(@"[DEBUG][networkProbeWithCompletion] error!! %@", error);
                completion(false,error);
			} else {
				//TODO: do we actually care about the contents of the file?
				completion(true, error);
			}
		}];
        [task resume];
    });
}


@end
