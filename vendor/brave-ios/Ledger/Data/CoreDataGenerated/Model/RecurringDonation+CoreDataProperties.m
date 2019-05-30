//
//  RecurringDonation+CoreDataProperties.m
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "RecurringDonation+CoreDataProperties.h"

@implementation RecurringDonation (CoreDataProperties)

+ (NSFetchRequest<RecurringDonation *> *)fetchRequest {
	return [NSFetchRequest fetchRequestWithEntityName:@"RecurringDonation"];
}

@dynamic addedDate;
@dynamic amount;
@dynamic publisherID;
@dynamic publisher;

@end
