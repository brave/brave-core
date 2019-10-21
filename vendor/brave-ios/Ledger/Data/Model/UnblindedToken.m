//
//  UnblindedToken+CoreDataClass.m
//  
//
//  Created by Kyle Hickinson on 2019-10-21.
//
//

#import "UnblindedToken.h"

@implementation UnblindedToken

+ (NSFetchRequest<UnblindedToken *> *)fetchRequest {
  return [NSFetchRequest fetchRequestWithEntityName:@"UnblindedToken"];
}

@dynamic tokenID;
@dynamic publicKey;
@dynamic value;
@dynamic promotion;

@end
