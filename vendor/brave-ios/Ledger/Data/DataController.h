/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

NS_ASSUME_NONNULL_BEGIN

@interface DataController : NSObject

@property (nonatomic, class) DataController *shared;

@property (nonatomic, readonly) NSURL *storeURL;

- (void)addPersistentStoreForContainer:(NSPersistentContainer *)container;

@property (nonatomic, readonly) BOOL storeExists;

/// Context object also allows us access to all persistent container data if needed.
+ (NSManagedObjectContext *)viewContext;

+ (NSManagedObjectContext *)newBackgroundContext;

+ (void)save:(NSManagedObjectContext *)context;

- (void)performOnContext:(nullable NSManagedObjectContext *)context task:(void (^)(NSManagedObjectContext *))task;
- (void)performOnContext:(nullable NSManagedObjectContext *)context save:(BOOL)save task:(void (^)(NSManagedObjectContext *))task;

@end

NS_ASSUME_NONNULL_END
