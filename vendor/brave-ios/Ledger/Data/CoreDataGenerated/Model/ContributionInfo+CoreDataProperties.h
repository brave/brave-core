//
//  ContributionInfo+CoreDataProperties.h
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "ContributionInfo+CoreDataClass.h"


NS_ASSUME_NONNULL_BEGIN

@interface ContributionInfo (CoreDataProperties)

+ (NSFetchRequest<ContributionInfo *> *)fetchRequest;

@property (nonatomic) int32_t category;
@property (nonatomic) int64_t date;
@property (nonatomic) int32_t month;
@property (nullable, nonatomic, copy) NSString *probi;
@property (nullable, nonatomic, copy) NSString *publisherID;
@property (nonatomic) int32_t year;
@property (nullable, nonatomic, retain) PublisherInfo *publisher;

@end

NS_ASSUME_NONNULL_END
