// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

@import UserNotifications;
#import "GRDServerManager.h"
#import "NSLogDisabler.h"
#import "NSPredicate+Additions.h"

@interface GRDServerManager() {
    GRDNetworkHealthType networkHealth;
}
@property GRDHousekeepingAPI *housekeeping;
@end

@implementation GRDServerManager

- (instancetype)init {
    if (self = [super init]) {
        self.housekeeping = [[GRDHousekeepingAPI alloc] init];
    }
    return self;
}

- (void)selectGuardianHostWithCompletion:(void (^)(NSString * _Nullable, NSString * _Nullable, NSString * _Nullable errorMessage))completion {
    [self getGuardianHostsWithCompletion:^(NSArray * _Nullable servers, NSString * _Nullable errorMessage) {
        if (servers == nil) {
            if (completion) completion(nil, nil, errorMessage);
            return;
        }
        
        // The server selection logic tries to prioritize low capacity servers which is defined as
        // having few clients connected. Low is defined as a capacity score of 0 or 1
        // capcaity score != connected clients. It's a calculated value based on information from each VPN node
        // this predicate will filter out anything above 1 as its capacity score
        NSArray *availableServers = [servers filteredArrayUsingPredicate:[NSPredicate capacityPredicate]];
        
        // if at least 2 low capacity servers are not available, just use full list instead
        // helps mitigate edge case: single server returned, but it is down yet not reported as such by Housekeeping
        if ([availableServers count] < 2) {
            // take full list of servers returned by housekeeping and use them
            availableServers = servers;
            NSLog(@"[selectGuardianHostWithCompletion] less than 2 low cap servers available, so not limiting list");
        }
        
        // Get a random index based on the length of availableServers
        // Then use that random index to select a hostname and return it to the caller
        NSUInteger randomIndex = arc4random_uniform((unsigned int)[availableServers count]);
        NSString *host = [[availableServers objectAtIndex:randomIndex] objectForKey:@"hostname"];
        NSString *hostLocation = [[availableServers objectAtIndex:randomIndex] objectForKey:@"display-name"];
        NSLog(@"Selected hostname: %@", host);
        if (completion) completion(host, hostLocation, nil);
    }];
}

- (void)getGuardianHostsWithCompletion:(void (^)(NSArray * _Nullable, NSString * _Nullable))completion {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    
    // Grab the timestamp out NSUserDefaults of the last time we checked all known time zones
    // If it hasn't been set before or removed out of the local cache set it to 0 so that
    // housekeeping returns all time zones
    NSNumber *timestamp = [defaults objectForKey:@"housekeepingTimezonesTimestamp"];
    if (timestamp == nil) {
        timestamp = [NSNumber numberWithInt:0];
    }
    
    [self.housekeeping requestTimeZonesForRegionsWithTimestamp:timestamp completion:^(NSArray * _Nonnull timeZones, BOOL success, NSUInteger responseStatusCode) {
        if (success == NO) {
            NSLog(@"Failed to get timezones from housekeeping: %ld", responseStatusCode);
            
            // In case housekeeping goes down try to grab the cached hosts
            NSArray *knownServersForRegion = [defaults objectForKey:@"kKnownGuardianHosts"];
            if (knownServersForRegion != nil) {
                NSLog(@"Found cached array of servers");
                if (completion) completion(knownServersForRegion, nil);
                return;
            } else {
                if (completion) completion(nil, @"Failed to request list of servers");
                return;
            }
            
        } else if (timeZones == nil && responseStatusCode == 304) {
            // No new time zones to process. Using the existing servers we have cached already
            NSArray *knownServersForRegion = [defaults objectForKey:@"kKnownGuardianHosts"];
            if (completion) completion(knownServersForRegion, nil);
            return;
        }
        
        // Saving the list of new time zones in NSUserDefaults for later use
        [defaults setObject:timeZones forKey:@"kKnownHousekeepingTimeZonesForRegions"];
        
        // Setting the timestamp so that housekeeping doesn't always return the list of time zones
        // again, but simply a few HTTP headers
        NSTimeInterval nowUnix = [[NSDate date] timeIntervalSince1970];
        [defaults setObject:[NSNumber numberWithInt:nowUnix] forKey:@"housekeepingTimezonesTimestamp"];
        
        NSDictionary *region = [GRDServerManager localRegionFromTimezones:timeZones];
        NSLog(@"[DEBUG] found region: %@", region[@"name"]);
        NSString *regionName = region[@"name"];
        NSTimeZone *local = [NSTimeZone localTimeZone];
        NSLog(@"[DEBUG] real local time zone: %@", local);
        
//        if ([defaults boolForKey:kGuardianUseFauxTimeZone]){
//            NSLog(@"[DEBUG] using faux timezone: %@", [defaults valueForKey:kGuardianFauxTimeZone]);
//            regionName = [defaults valueForKey:kGuardianFauxTimeZone];
//        }
        // This is only meant as a fallback to have something
        // when absolutely everything seems to have fallen apart
        if (regionName == nil) {
            NSLog(@"[getGuardianHostsWithCompletion] Failed to find time zone: %@", local);
            NSLog(@"[getGuardianHostsWithCompletion] Setting time zone to us-east");
            regionName = @"us-east";
        }
        
        [self.housekeeping requestServersForRegion:regionName completion:^(NSArray * _Nonnull servers, BOOL success) {
            if (success == false) {
                NSLog(@"[getGuardianHostsWithCompletion] Failed to get servers for region");
                if (completion) completion(nil, @"Failed to request list of servers.");
                return;
            } else {
                //NSLog(@"servers: %@", servers);
                [defaults setObject:servers forKey:@"kKnownGuardianHosts"];
                if (completion) completion(servers, nil);
            }
        }];
    }];
}

- (void)findBestHostInRegion:(NSString *)regionName
                  completion:(void(^_Nullable)(NSString * _Nullable host,
                                               NSString *hostLocation,
                                               NSString * _Nullable error))block {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        GRDHousekeepingAPI *housekeeping = [[GRDHousekeepingAPI alloc] init];
        [housekeeping requestServersForRegion:regionName completion:^(NSArray * _Nonnull servers, BOOL success) {
            NSLog(@"[DEBUG] servers: %@", servers);
            if (servers.count < 1){
                if (block){
                    dispatch_async(dispatch_get_main_queue(), ^{
                        block(nil, nil, NSLocalizedString(@"No server found", nil));
                    });
                }
            } else {
                NSArray *availableServers = [servers filteredArrayUsingPredicate:[NSPredicate capacityPredicate]];
                NSLog(@"[DEBUG] availableServers: %@", availableServers);
                if (availableServers.count < 2){
                    NSLog(@"[DEBUG] less than 2 low capacity servers: %@", availableServers);
                    availableServers = servers;
                }
                
                NSUInteger randomIndex = arc4random_uniform((unsigned int)[availableServers count]);
                NSString *guardianHost = [[availableServers objectAtIndex:randomIndex] objectForKey:@"hostname"];
                NSString *guardianHostLocation = [[availableServers objectAtIndex:randomIndex] objectForKey:@"display-name"];
                NSLog(@"[DEBUG] selecting host: %@ at random index: %lu", guardianHost, randomIndex);
                if(block){
                    dispatch_async(dispatch_get_main_queue(), ^{
                        block(guardianHost, guardianHostLocation, nil);
                    });
                }
            }
        }];
    });
}



+ (NSDictionary *)localRegionFromTimezones:(NSArray *)timezones {
    NSDictionary *found = [[timezones filteredArrayUsingPredicate:[NSPredicate timezonePredicate]] lastObject];
    return found;
    
}

@end
