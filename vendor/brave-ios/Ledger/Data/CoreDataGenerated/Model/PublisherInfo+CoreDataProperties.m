//
//  PublisherInfo+CoreDataProperties.m
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "PublisherInfo+CoreDataProperties.h"

@implementation PublisherInfo (CoreDataProperties)

+ (NSFetchRequest<PublisherInfo *> *)fetchRequest {
	return [NSFetchRequest fetchRequestWithEntityName:@"PublisherInfo"];
}

@dynamic excluded;
@dynamic faviconURL;
@dynamic name;
@dynamic provider;
@dynamic publisherID;
@dynamic url;
@dynamic verified;
@dynamic activities;
@dynamic contributions;
@dynamic recurringDonations;

@end
