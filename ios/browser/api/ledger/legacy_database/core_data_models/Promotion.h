//
//  Promotion+CoreDataClass.h
//
//
//  Created by Kyle Hickinson on 2019-10-21.
//
//

#import <CoreData/CoreData.h>
#import <Foundation/Foundation.h>

@class PromotionCredentials;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface Promotion : NSManagedObject

+ (NSFetchRequest<Promotion*>*)fetchRequest;

@property(nonatomic, copy) NSString* promotionID;
@property(nonatomic) int32_t version;
@property(nonatomic) int32_t type;
@property(nonatomic, copy) NSString* publicKeys;
@property(nonatomic) int32_t suggestions;
@property(nonatomic) double approximateValue;
@property(nonatomic) int32_t status;
@property(nonatomic, copy) NSDate* expiryDate;

@end

NS_ASSUME_NONNULL_END
