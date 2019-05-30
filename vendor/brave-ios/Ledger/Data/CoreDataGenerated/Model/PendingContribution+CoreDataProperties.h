//
//  PendingContribution+CoreDataProperties.h
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "PendingContribution+CoreDataClass.h"


NS_ASSUME_NONNULL_BEGIN

@interface PendingContribution (CoreDataProperties)

+ (NSFetchRequest<PendingContribution *> *)fetchRequest;

@property (nonatomic) int64_t addedDate;
@property (nonatomic) double amount;
@property (nonatomic) int32_t category;
@property (nullable, nonatomic, copy) NSString *publisherID;
@property (nullable, nonatomic, copy) NSString *viewingID;

@end

NS_ASSUME_NONNULL_END
