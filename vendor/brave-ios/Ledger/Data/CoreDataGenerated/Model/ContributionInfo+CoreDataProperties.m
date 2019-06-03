//
//  ContributionInfo+CoreDataProperties.m
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "ContributionInfo+CoreDataProperties.h"

@implementation ContributionInfo (CoreDataProperties)

+ (NSFetchRequest<ContributionInfo *> *)fetchRequest {
	return [NSFetchRequest fetchRequestWithEntityName:@"ContributionInfo"];
}

@dynamic category;
@dynamic date;
@dynamic month;
@dynamic probi;
@dynamic publisherID;
@dynamic year;
@dynamic publisher;

@end
