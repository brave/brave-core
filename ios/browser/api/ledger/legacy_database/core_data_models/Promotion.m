//
//  Promotion+CoreDataClass.m
//
//
//  Created by Kyle Hickinson on 2019-10-21.
//
//

#import "Promotion.h"

@implementation Promotion

+ (NSFetchRequest<Promotion*>*)fetchRequest {
  return [NSFetchRequest fetchRequestWithEntityName:@"Promotion"];
}

@dynamic promotionID;
@dynamic version;
@dynamic type;
@dynamic publicKeys;
@dynamic suggestions;
@dynamic approximateValue;
@dynamic status;
@dynamic expiryDate;

@end
