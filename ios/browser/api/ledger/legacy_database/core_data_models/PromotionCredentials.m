//
//  PromotionCredentials+CoreDataClass.m
//
//
//  Created by Kyle Hickinson on 2019-10-21.
//
//

#import "PromotionCredentials.h"

@implementation PromotionCredentials

+ (NSFetchRequest<PromotionCredentials*>*)fetchRequest {
  return [NSFetchRequest fetchRequestWithEntityName:@"PromotionCredentials"];
}

@dynamic blindedCredentials;
@dynamic signedCredentials;
@dynamic publicKey;
@dynamic batchProof;
@dynamic claimID;
@dynamic tokens;
@dynamic promotionID;

@end
