//
//  RecurringDonation+CoreDataProperties.h
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "RecurringDonation+CoreDataClass.h"


NS_ASSUME_NONNULL_BEGIN

@interface RecurringDonation (CoreDataProperties)

+ (NSFetchRequest<RecurringDonation *> *)fetchRequest;

@property (nonatomic) int64_t addedDate;
@property (nonatomic) double amount;
@property (nullable, nonatomic, copy) NSString *publisherID;
@property (nullable, nonatomic, retain) PublisherInfo *publisher;

@end

NS_ASSUME_NONNULL_END
