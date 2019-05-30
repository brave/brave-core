//
//  ActivityInfo+CoreDataProperties.h
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "ActivityInfo+CoreDataClass.h"


NS_ASSUME_NONNULL_BEGIN

@interface ActivityInfo (CoreDataProperties)

+ (NSFetchRequest<ActivityInfo *> *)fetchRequest;

@property (nonatomic) int64_t duration;
@property (nonatomic) int32_t percent;
@property (nullable, nonatomic, copy) NSString *publisherID;
@property (nonatomic) int64_t reconcileStamp;
@property (nonatomic) double score;
@property (nonatomic) int32_t visits;
@property (nonatomic) double weight;
@property (nullable, nonatomic, retain) PublisherInfo *publisher;

@end

NS_ASSUME_NONNULL_END
