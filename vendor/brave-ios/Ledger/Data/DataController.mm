/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "DataController.h"

@interface DataController ()
@property (nonatomic) NSOperationQueue *operationQueue;
@property (nonatomic) NSPersistentContainer *container;
@end

@implementation DataController

static DataController *_dataController = nil;

+ (DataController *)shared
{
  if (!_dataController) {
    _dataController = [[DataController alloc] init];
  }
  return _dataController;
}

+ (void)setShared:(DataController *)shared
{
  _dataController = shared;
}

- (NSURL *)storeURL
{
  const auto urls = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  const auto documentURL = urls.lastObject;
  if (!documentURL) {
    return nil;
  }
  return [NSURL fileURLWithPath:[documentURL stringByAppendingPathComponent:@"BraveRewards.sqlite"]];
}

- (instancetype)init
{
  if ((self = [super init])) {
    self.operationQueue = [[NSOperationQueue alloc] init];
    self.operationQueue.maxConcurrentOperationCount = 1;
    
    // Setup container
    const auto bundle = [NSBundle bundleForClass:DataController.class];
    const auto modelURL = [bundle URLForResource:@"Model" withExtension:@"momd"];
    NSAssert(modelURL != nil, @"Error loading model from bundle");
    const auto model = [[NSManagedObjectModel alloc] initWithContentsOfURL:modelURL];
    NSAssert(model != nil, @"Error initializing managed object model from: %@", modelURL);
    self.container = [[NSPersistentContainer alloc] initWithName:@"Model"
                                              managedObjectModel:model];
    [self addPersistentStoreForContainer:self.container];
    [self.container loadPersistentStoresWithCompletionHandler:^(NSPersistentStoreDescription * _Nonnull, NSError * _Nullable error) {
      NSAssert(error == nil, @"Load persistent store error: %@", error);
    }];
    self.container.viewContext.automaticallyMergesChangesFromParent = YES;
  }
  return self;
}

- (void)addPersistentStoreForContainer:(NSPersistentContainer *)container
{
  // This makes the database file encrypted until device is unlocked.
  const auto storeDescription = [[NSPersistentStoreDescription alloc] initWithURL:self.storeURL];
  [storeDescription setOption:NSFileProtectionComplete forKey:NSPersistentStoreFileProtectionKey];
  self.container.persistentStoreDescriptions = @[storeDescription];
}

- (BOOL)storeExists
{
  return [NSFileManager.defaultManager fileExistsAtPath:self.storeURL.path];
}

+ (NSManagedObjectContext *)newBackgroundContext
{
  const auto backgroundContext = [DataController.shared.container newBackgroundContext];
  // In theory, the merge policy should not matter
  // since all operations happen on a synchronized operation queue.
  // But in case of any bugs it's better to have one, so the app won't crash for users.
  backgroundContext.mergePolicy = NSMergeByPropertyStoreTrumpMergePolicy;
  return backgroundContext;
}

+ (NSManagedObjectContext *)viewContext {
  return DataController.shared.container.viewContext;
}

+ (void)save:(NSManagedObjectContext *)context
{
  if (context == DataController.viewContext) {
    NSLog(@"Writing to view context, this should be avoided.");
  }
  [context performBlock:^{
    if (!context.hasChanges) { return; }
    NSError *error;
    if (![context save:&error]) {
      NSLog(@"Error saving DB: %@", error);
    }
  }];
}

- (void)performOnContext:(NSManagedObjectContext *)context task:(void (^)(NSManagedObjectContext * _Nonnull))task
{
  [self performOnContext:context save:YES task:task];
}

- (void)performOnContext:(NSManagedObjectContext *)context save:(BOOL)save task:(void (^)(NSManagedObjectContext * _Nonnull))task
{
  // If existing context is provided, we only call the code closure.
  // Queue operation and saving is done in `performTask()` called at higher level when a nil context
  // is passed
  if (context) {
    task(context);
    return;
  }
  
  [self.operationQueue addOperationWithBlock:^{
    const auto backgroundContext = [DataController newBackgroundContext];
    // performAndWait doesn't block main thread because it fires on OperationQueue`s background thread.
    [backgroundContext performBlockAndWait:^{
      task(backgroundContext);
      
      if (save && backgroundContext.hasChanges) {
        assert(![NSThread isMainThread]);
        NSError *error;
        if (![backgroundContext save:&error]) {
          NSLog(@"performTask save error: %@", error);
        }
      }
    }];
  }];
}

@end
