//
//  ActivityInfo+CoreDataProperties.m
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "ActivityInfo+CoreDataProperties.h"

@implementation ActivityInfo (CoreDataProperties)

+ (NSFetchRequest<ActivityInfo *> *)fetchRequest {
	return [NSFetchRequest fetchRequestWithEntityName:@"ActivityInfo"];
}

@dynamic duration;
@dynamic percent;
@dynamic publisherID;
@dynamic reconcileStamp;
@dynamic score;
@dynamic visits;
@dynamic weight;
@dynamic publisher;

@end
