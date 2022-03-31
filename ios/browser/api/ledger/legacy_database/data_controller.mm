/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "data_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface DataController ()
@property(nonatomic) NSOperationQueue* operationQueue;
@property(nonatomic) NSPersistentContainer* container;
@end

@implementation DataController

static DataController* _dataController = nil;

+ (DataController*)shared {
  if (!_dataController) {
    _dataController = [[DataController alloc] init];
  }
  return _dataController;
}

+ (void)setShared:(DataController*)shared {
  _dataController = shared;
}

- (NSURL*)storeDirectoryURL {
  const auto urls = NSSearchPathForDirectoriesInDomains(
      NSApplicationSupportDirectory, NSUserDomainMask, YES);
  const auto documentURL = urls.lastObject;
  if (!documentURL) {
    return nil;
  }
  return [NSURL
      fileURLWithPath:[documentURL stringByAppendingPathComponent:@"rewards"]];
}

- (NSURL*)storeURL {
  return [[self storeDirectoryURL]
      URLByAppendingPathComponent:@"BraveRewards.sqlite"];
}

+ (BOOL)defaultStoreExists {
  const auto urls = NSSearchPathForDirectoriesInDomains(
      NSApplicationSupportDirectory, NSUserDomainMask, YES);
  const auto documentURL = urls.lastObject;
  if (!documentURL) {
    return NO;
  }
  const auto directoryURL = [NSURL
      fileURLWithPath:[documentURL stringByAppendingPathComponent:@"rewards"]];
  const auto storeURL =
      [directoryURL URLByAppendingPathComponent:@"BraveRewards.sqlite"];
  return [NSFileManager.defaultManager fileExistsAtPath:storeURL.path];
}

- (instancetype)init {
  if ((self = [super init])) {
    self.operationQueue = [[NSOperationQueue alloc] init];
    self.operationQueue.maxConcurrentOperationCount = 1;

    [[NSFileManager defaultManager]
               createDirectoryAtURL:[self storeDirectoryURL]
        withIntermediateDirectories:YES
                         attributes:nil
                              error:nil];

    // Setup container
    const auto bundle = [NSBundle bundleForClass:DataController.class];
    const auto modelURL = [bundle URLForResource:@"Model"
                                   withExtension:@"momd"];
    NSAssert(modelURL != nil, @"Error loading model from bundle");
    const auto model =
        [[NSManagedObjectModel alloc] initWithContentsOfURL:modelURL];
    NSAssert(model != nil, @"Error initializing managed object model from: %@",
             modelURL);
    self.container = [[NSPersistentContainer alloc] initWithName:@"Model"
                                              managedObjectModel:model];
    [self addPersistentStoreForContainer:self.container];
    [self.container
        loadPersistentStoresWithCompletionHandler:^(
            NSPersistentStoreDescription* _Nonnull, NSError* _Nullable error) {
          NSAssert(error == nil, @"Load persistent store error: %@", error);
        }];
    self.container.viewContext.automaticallyMergesChangesFromParent = YES;
  }
  return self;
}

- (void)addPersistentStoreForContainer:(NSPersistentContainer*)container {
  // This makes the database file encrypted until device is unlocked.
  const auto storeDescription =
      [[NSPersistentStoreDescription alloc] initWithURL:self.storeURL];
  [storeDescription setOption:NSFileProtectionComplete
                       forKey:NSPersistentStoreFileProtectionKey];
  self.container.persistentStoreDescriptions = @[ storeDescription ];
}

+ (NSManagedObjectContext*)newBackgroundContext {
  const auto backgroundContext =
      [DataController.shared.container newBackgroundContext];
  // In theory, the merge policy should not matter
  // since all operations happen on a synchronized operation queue.
  // But in case of any bugs it's better to have one, so the app won't crash for
  // users.
  backgroundContext.mergePolicy = NSMergeByPropertyStoreTrumpMergePolicy;
  return backgroundContext;
}

+ (NSManagedObjectContext*)viewContext {
  return DataController.shared.container.viewContext;
}

@end
