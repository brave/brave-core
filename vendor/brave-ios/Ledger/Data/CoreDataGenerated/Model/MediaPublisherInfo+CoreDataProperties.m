//
//  MediaPublisherInfo+CoreDataProperties.m
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "MediaPublisherInfo+CoreDataProperties.h"

@implementation MediaPublisherInfo (CoreDataProperties)

+ (NSFetchRequest<MediaPublisherInfo *> *)fetchRequest {
	return [NSFetchRequest fetchRequestWithEntityName:@"MediaPublisherInfo"];
}

@dynamic mediaKey;
@dynamic publisherID;

@end
