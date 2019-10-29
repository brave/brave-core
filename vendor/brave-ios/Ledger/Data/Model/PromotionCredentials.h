//
//  PromotionCredentials+CoreDataClass.h
//  
//
//  Created by Kyle Hickinson on 2019-10-21.
//
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class Promotion;

NS_ASSUME_NONNULL_BEGIN

@interface PromotionCredentials : NSManagedObject

+ (NSFetchRequest<PromotionCredentials *> *)fetchRequest;

@property (nonatomic, copy) NSString *blindedCredentials;
@property (nullable, nonatomic, copy) NSString *signedCredentials;
@property (nullable, nonatomic, copy) NSString *publicKey;
@property (nullable, nonatomic, copy) NSString *batchProof;
@property (nonatomic, copy) NSString *claimID;
@property (nullable, nonatomic, retain) Promotion *promotion;
@property (nonatomic, copy) NSString *tokens;

@end

NS_ASSUME_NONNULL_END
