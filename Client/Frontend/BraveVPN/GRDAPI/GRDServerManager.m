// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "GRDServerManager.h"
#import "NSLogDisabler.h"

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

- (void)selectGuardianHostWithCompletion:(void (^)(NSString * _Nullable, NSString * _Nullable errorMessage))completion {
    [self getGuardianHostsWithCompletion:^(NSArray * _Nullable servers, NSString * _Nullable errorMessage) {
        if (servers == nil) {
            if (completion) completion(nil, errorMessage);
            return;
        }
        
        // Create two mutable arrays for later use
        NSMutableArray *zeroServers = [[NSMutableArray alloc] init];
        NSMutableArray *oneServers = [[NSMutableArray alloc] init];
        
        for (int i = 0; i < [servers count]; i++) {
            NSDictionary *serverObj = [servers objectAtIndex:i];
            
            // Seperate the available servers into capacity score of 0 and 1
            // Capacity scores over 1 are ignored entirely
            if ([[serverObj objectForKey:@"capacity-score"] integerValue] == 0) {
                [zeroServers addObject:serverObj];
            
            } else if ([[serverObj objectForKey:@"capacity-score"] integerValue] == 1) {
                [oneServers addObject:serverObj];
            }
        }
        
        NSArray *availableServers;
        // Fallback in the case that there are no servers with capacity score of 0 or 1
        if ([zeroServers count] == 0 && [oneServers count] == 0) {
            // Just take the servers returned by housekeeping and send it
            availableServers = [NSArray arrayWithArray:servers];
            
        } else {
            // If there is only 1 or 0 servers available with a 0 capacity score
            // add it to the oneServers array and use it
            if ([zeroServers count] <= 1) {
                for (NSDictionary *zeroServer in zeroServers) {
                    [oneServers addObject:zeroServer];
                }
                availableServers = [NSArray arrayWithArray:oneServers];
            
            // If there are at least two servers with a capcity score of 0 use that array
            } else if ([zeroServers count] > 1) {
                availableServers = [NSArray arrayWithArray:zeroServers];
            
            // If there are only servers with a capacity score of 1 use the oneServers array
            } else {
                availableServers = [NSArray arrayWithArray:oneServers];
            }
        }
        
        // Get a random index based on the length of availableServers
        // Then use that random index to select a hostname and return it to the caller
        NSUInteger randomIndex = arc4random_uniform((unsigned int)[availableServers count]);
        NSString *host = [[availableServers objectAtIndex:randomIndex] objectForKey:@"hostname"];
        NSLog(@"Selected hostname: %@", host);
        if (completion) completion(host, nil);
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
        
        NSString *regionName;
        BOOL regionFound = NO;
        NSTimeZone *local = [NSTimeZone localTimeZone];
        
        // Loop through all time zones to match it with the current local
        // one the device is currently set to
        for (NSDictionary *region in timeZones) {
            NSString *localRegionName = [region objectForKey:@"name"];
            NSArray *regionTimezones = [region objectForKey:@"timezones"];
            
            // Loop through all time zones for the current region
            for (NSString *timezone in regionTimezones) {
                if ([[local name] isEqualToString:timezone]) {
                    regionName = localRegionName;
                    regionFound = YES;
                    break;
                }
                
                // There are time zones like "Cupertino" in iOS which is not a
                // real time zone but a pseudo time zone. Simply set the region to us-west
                if ([[local name] isEqualToString:@"US/Pacific"]) {
                    regionName = @"us-west";
                    regionFound = YES;
                    break;
                }
            }
            
            // Exit the outter loop
            if (regionFound == YES) {
                break;
            }
        }
        
        // This is only meant as a fallback to have something
        // when absolutely everything seems to have fallen apart
        if (regionFound == NO) {
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
                [defaults setObject:servers forKey:@"kKnownGuardianHosts"];
                if (completion) completion(servers, nil);
            }
        }];
    }];
}

@end
