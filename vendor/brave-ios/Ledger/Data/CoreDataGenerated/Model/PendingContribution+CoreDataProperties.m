//
//  PendingContribution+CoreDataProperties.m
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "PendingContribution+CoreDataProperties.h"

@implementation PendingContribution (CoreDataProperties)

+ (NSFetchRequest<PendingContribution *> *)fetchRequest {
	return [NSFetchRequest fetchRequestWithEntityName:@"PendingContribution"];
}

@dynamic addedDate;
@dynamic amount;
@dynamic category;
@dynamic publisherID;
@dynamic viewingID;

@end
