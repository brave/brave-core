// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>
#import <DeviceCheck/DeviceCheck.h>

#import "GRDKeychain.h"
// BRAVE TODO: Commented out this line
//#import "GRDBlacklistItem.h"
#import "GRDGatewayAPIResponse.h"

#define kSGAPI_ValidateReceipt_APIv1 @"/api/v1/verify-receipt"

#define kSGAPI_DefaultHostname @"us-west-1.sudosecuritygroup.com"
#define kSGAPI_Register @"/vpnsrv/api/register"
#define kSGAPI_SignIn @"/vpnsrv/api/signin"
#define kSGAPI_SignOut @"/vpnsrv/api/signout"
#define kSGAPI_ValidateReceipt @"/vpnsrv/api/verify-receipt"
#define kSGAPI_ServerStatus @"/vpnsrv/api/server-status"

#define kSGAPI_DeviceBase @"/vpnsrv/api/device"
#define kSGAPI_Device_Create @"/create"
#define kSGAPI_Device_SetPushToken @"/set-push-token"
#define kSGAPI_Device_GetAlerts @"/alerts"
#define kSGAPI_Device_EAP_GetCreds @"/eap-credentials"
#define kSGAPI_Device_EAP_RegenerateCreds @"/regenerate-eap-credentials"
#define kSGAPI_Device_GetPointOfAccess @"/get-point-of-access"
#define kGSAPI_Rule_AddDNS @"/rule/add-dns"
#define kGSAPI_Rule_AddIP @"/rule/add-ip"
#define kGSAPI_Rule_Delete @"/rule/delete"


typedef NS_ENUM(NSInteger, GRDNetworkHealthType) {
    GRDNetworkHealthUnknown = 0,
    GRDNetworkHealthBad,
    GRDNetworkHealthGood
};

NS_ASSUME_NONNULL_BEGIN

@interface GRDGatewayAPI : NSObject

/// can be set to true to make - (void)getEvents return dummy alerts for debgging purposes
@property BOOL dummyDataForDebugging;

/// apiAuthToken is used as a second factor of authentication by the zoe-agent API. zoe-agent expects this value to be sent in the JSON encoded body of the HTTP request for the value 'api-auth-token'
@property (strong, nonatomic, setter=setAPIAuthToken:) NSString *apiAuthToken;

/// deviceIdentifier and eapUsername are the same values. eapUsername is stored in the keychain for the value 'eap-username'
@property (strong, nonatomic) NSString *deviceIdentifier;

/// apiHostname holds the value of the zoe-agent instance the app is currently connected to in memory. A persistent copy of it is stored in NSUserDefaults
@property (strong, nonatomic) NSString *apiHostname;

/// timer used to regularly check on the network condition and detect network changes or outages
@property (strong, nonatomic) NSTimer * _Nullable healthCheckTimer;


/// singleton object to quickly access objects from the VPN host
+ (instancetype)sharedAPI;

/// hits an endpoint with as little data transferred as possible to verify that network requests can still be made
- (void)networkHealthCheck;

/// convenience method to start healthCheckTimer at a preset interval
- (void)startHealthCheckTimer;

/// convencience method to stop healthCheckTimer
- (void)stopHealthCheckTimer;

/// hits endpoint to probe current network health
- (void)networkProbeWithCompletion:(void (^)(BOOL status, NSError *error))completion ;

/// retrieves values out of the system keychain and stores them in the sharedAPI singleton object in memory for other functions to use in the future
- (void)_loadCredentialsFromKeychain;

/// DEPRECATED! All URL encoding has been removed from zoe-agent. DO NOT USE!
- (NSMutableURLRequest *)_requestWithEndpoint:(NSString *)apiEndpoint andPostRequestString:(NSString *)postRequestStr;

/// convenience method to quickly set various HTTP headers
- (NSMutableURLRequest *)_requestWithEndpoint:(NSString *)apiEndpoint andPostRequestData:(NSData *)postRequestDat;

/// endpoint: /vpnsrv/api/server-status
/// hits the endpoint for the current VPN host to check if a VPN connection can be established
- (void)getServerStatusWithCompletion:(void (^)(GRDGatewayAPIResponse *apiResponse))completion;

/// endpoint: /api/v1.1/register-and-create
/// @param subscriberCredential JWT token obtained from housekeeping
/// @param completion completion block indicating success, returning EAP Credentials as well as an API auth token and reporting a user actional error message back to the caller
- (void)registerAndCreateWithSubscriberCredential:(NSString *)subscriberCredential completion:(void (^)(NSDictionary * _Nullable credentials, BOOL success, NSString * _Nullable errorMessage))completion;

- (void)bindPushToken:(NSString *)pushTok notificationMode:(NSString *)notifMode;

/// endpoint: "/api/v1.1/device/<device_token>/alerts"
/// @param completion De-Serialized JSON from the server containing an array with all alerts
- (void)getEvents:(void (^)(NSDictionary *response, BOOL success, NSString *error))completion;

@end

NS_ASSUME_NONNULL_END

