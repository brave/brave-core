//
//  NSPredicate+Additions.m
//  Guardian
//
//  Created by Kevin Bradley on 8/19/20.
//  Copyright © 2020 Sudo Security Group Inc. All rights reserved.
//

#import "NSPredicate+Additions.h"

@implementation NSPredicate (Additions)

+ (NSPredicate *)timezonePredicate {
    NSString *local = [[NSTimeZone localTimeZone] name];
    NSPredicate *pred = [NSPredicate predicateWithBlock:^BOOL(id  _Nullable evaluatedObject, NSDictionary<NSString *,id> * _Nullable bindings) {
        
        NSArray *timezones = [evaluatedObject valueForKey:@"timezones"];
        if ([timezones containsObject:local]){
            //NSLog(@"evaluatedObject: %@ has %@", evaluatedObject, local);
            return TRUE;
        }
        return FALSE;
        
    }];
    if ([local isEqualToString:@"US/Pacific"]){ //Cupertino
        pred = [NSPredicate predicateWithFormat:@"name == %@", @"us-west"];
    }
    return pred;
}

+ (NSPredicate *)capacityPredicate {
    return [NSPredicate predicateWithBlock:^BOOL(id  _Nullable evaluatedObject, NSDictionary<NSString *,id> * _Nullable bindings) {
        NSInteger cs = [evaluatedObject[@"capacity-score"] integerValue];
        if (cs <= 1){
            return TRUE;
        }
        return FALSE;
    }];
}

@end
