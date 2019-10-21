//
//  UnblindedToken+CoreDataClass.h
//  
//
//  Created by Kyle Hickinson on 2019-10-21.
//
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class Promotion;

NS_ASSUME_NONNULL_BEGIN

@interface UnblindedToken : NSManagedObject

+ (NSFetchRequest<UnblindedToken *> *)fetchRequest;

@property (nonatomic) int64_t tokenID;
@property (nullable, nonatomic, copy) NSString *publicKey;
@property (nonatomic) double value;
@property (nullable, nonatomic, retain) Promotion *promotion;

@end

NS_ASSUME_NONNULL_END
