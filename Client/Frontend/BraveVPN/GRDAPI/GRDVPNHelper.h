// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>

#import "GRDKeychain.h"
#import "GRDGatewayAPI.h"
#import "GRDServerManager.h"
#import "GRDHousekeepingAPI.h"
//#import "GRDSettingsController.h"
#import "GRDGatewayAPIResponse.h"

NS_ASSUME_NONNULL_BEGIN

@interface GRDVPNHelper : NSObject

typedef NS_ENUM(NSInteger, GRDVPNHelperStatusCode) {
    GRDVPNHelperSuccess,
	GRDVPNHelperFail,
    GRDVPNHelperDoesNeedMigration,
	GRDVPNHelperMigrating,
    GRDVPNHelperNetworkConnectionError, // add other network errors
    GRDVPNHelperCoudNotReachAPIError,
    GRDVPNHelperApp_VpnPrefsLoadError,
    GRDVPNHelperApp_VpnPrefsSaveError,
    GRDVPNHelperAPI_AuthenticationError,
    GRDVPNHelperAPI_ProvisioningError
};

+ (BOOL)isPayingUser;
+ (void)setIsPayingUser:(BOOL)isPaying;
+ (void)clearVpnConfiguration;
+ (void)saveAllInOneBoxHostname:(NSString *)host;
+ (NEVPNProtocolIKEv2 *)prepareIKEv2ParametersForServer:(NSString *)server eapUsername:(NSString *)user eapPasswordRef:(NSData *)passRef withCertificateType:(NEVPNIKEv2CertificateType)certType;

- (void)configureAndConnectVPNWithCompletion:(void (^_Nullable)(NSString *_Nullable message, GRDVPNHelperStatusCode status))completion;
- (void)disconnectVPN;
- (void)createFreshUserWithSubscriberCredential:(NSString *)subscriberCredential completion:(void (^)(GRDVPNHelperStatusCode statusCode, NSString * _Nullable errString))completion;
+ (nullable NSString *)serverLocationForHostname:(NSString *)hostname;

@end

NS_ASSUME_NONNULL_END
