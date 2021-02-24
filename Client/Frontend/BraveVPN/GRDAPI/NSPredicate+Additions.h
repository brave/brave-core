//
//  NSPredicate+Additions.h
//  Guardian
//
//  Created by Kevin Bradley on 8/19/20.
//  Copyright © 2020 Sudo Security Group Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSPredicate (Additions)

+ (NSPredicate *)timezonePredicate;
+ (NSPredicate *)capacityPredicate;

@end

NS_ASSUME_NONNULL_END
