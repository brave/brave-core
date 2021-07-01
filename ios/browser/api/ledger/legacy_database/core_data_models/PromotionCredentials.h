//
//  PromotionCredentials+CoreDataClass.h
//
//
//  Created by Kyle Hickinson on 2019-10-21.
//
//

#import <CoreData/CoreData.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface PromotionCredentials : NSManagedObject

+ (NSFetchRequest<PromotionCredentials*>*)fetchRequest;

@property(nonatomic, copy) NSString* blindedCredentials;
@property(nullable, nonatomic, copy) NSString* signedCredentials;
@property(nullable, nonatomic, copy) NSString* publicKey;
@property(nullable, nonatomic, copy) NSString* batchProof;
@property(nonatomic, copy) NSString* claimID;
@property(nonatomic, copy) NSString* promotionID;
@property(nonatomic, copy) NSString* tokens;

@end

NS_ASSUME_NONNULL_END
