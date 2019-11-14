//
//  Promotion+CoreDataClass.h
//  
//
//  Created by Kyle Hickinson on 2019-10-21.
//
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class PromotionCredentials;

NS_ASSUME_NONNULL_BEGIN

@interface Promotion : NSManagedObject

+ (NSFetchRequest<Promotion *> *)fetchRequest;

@property (nonatomic, copy) NSString *promotionID;
@property (nonatomic) int32_t version;
@property (nonatomic) int32_t type;
@property (nonatomic, copy) NSString *publicKeys;
@property (nonatomic) int32_t suggestions;
@property (nonatomic) double approximateValue;
@property (nonatomic) BOOL claimed;
@property (nonatomic) BOOL active;
@property (nonatomic, copy) NSDate *expiryDate;

@end

NS_ASSUME_NONNULL_END
