//
//  UnblindedToken+CoreDataClass.h
//
//
//  Created by Kyle Hickinson on 2019-10-21.
//
//

#import <CoreData/CoreData.h>
#import <Foundation/Foundation.h>

@class Promotion;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface UnblindedToken : NSManagedObject

+ (NSFetchRequest<UnblindedToken*>*)fetchRequest;

@property(nonatomic) int64_t tokenID;
@property(nullable, nonatomic, copy) NSString* publicKey;
@property(nonatomic) double value;
@property(nullable, nonatomic, copy) NSString* promotionID;
@property(nullable, nonatomic, copy) NSString* tokenValue;

@end

NS_ASSUME_NONNULL_END
