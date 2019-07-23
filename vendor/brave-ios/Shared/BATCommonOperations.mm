/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATCommonOperations.h"

@interface BATCommonOperations ()
@property (nonatomic, copy) NSString *storagePath;
@property (nonatomic, assign) uint32_t currentTimerID;
@property (nonatomic, copy) NSMutableDictionary<NSNumber *, NSTimer *> *timers; // {ID: Timer}
@property (nonatomic, copy) NSMutableArray<NSURLSessionDataTask *> *runningTasks;
@end

@implementation BATCommonOperations

- (instancetype)initWithStoragePath:(NSString *)storagePath
{
  if ((self = [super init])) {
    self.storagePath = storagePath;
    _timers = [[NSMutableDictionary alloc] init];
    _runningTasks = [[NSMutableArray alloc] init];

    // Setup the ads directory for persistant storage
    if (self.storagePath.length > 0) {
      if (![NSFileManager.defaultManager fileExistsAtPath:self.storagePath isDirectory:nil]) {
        [NSFileManager.defaultManager createDirectoryAtPath:self.storagePath
                                withIntermediateDirectories:true
                                                 attributes:nil
                                                      error:nil];
      }
    }
  }
  return self;
}

- (instancetype)init
{
  return [self initWithStoragePath:nil];
}

- (void)dealloc
{
  [self.runningTasks makeObjectsPerformSelector:@selector(cancel)];
  for (NSNumber *timerID in self.timers) {
    [self.timers[timerID] invalidate];
  }
}

- (const std::string)generateUUID
{
  return std::string([NSUUID UUID].UUIDString.UTF8String);
}

- (uint32_t)createTimerWithOffset:(uint64_t)offset timerFired:(void (^)(uint32_t))timerFired
{
  self.currentTimerID++;
  const auto timerID = self.currentTimerID;

  auto const __weak weakSelf = self;
    
  dispatch_async(dispatch_get_main_queue(), ^{
      self.timers[[NSNumber numberWithUnsignedInt:timerID]] =
      [NSTimer scheduledTimerWithTimeInterval:offset repeats:false block:^(NSTimer * _Nonnull timer) {
          if (!weakSelf) { return; }
          timerFired(timerID);
      }];
  });
    
  
  return timerID;
}

- (void)removeTimerWithID:(uint32_t)timerID
{
  const auto key = [NSNumber numberWithUnsignedInt:timerID];
  const auto timer = self.timers[key];
  [timer invalidate];
  [self.timers removeObjectForKey:key];
}

- (void)loadURLRequest:(const std::string &)url headers:(const std::vector<std::string> &)headers content:(const std::string &)content content_type:(const std::string &)content_type method:(const std::string &)method callback:(BATNetworkCompletionBlock)callback
{
  const auto session = NSURLSession.sharedSession;
  const auto nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
  const auto request = [[NSMutableURLRequest alloc] initWithURL:nsurl];

  // At the moment `headers` is ignored, as I'm not sure how to use an array of strings to setup HTTP headers...

  if (content_type.length() > 0) {
    [request setValue:[NSString stringWithUTF8String:content_type.c_str()] forHTTPHeaderField:@"Content-Type"];
  }

  request.HTTPMethod = [NSString stringWithUTF8String:method.c_str()];

  if (method != "GET" && content.length() > 0) {
    // Assumed http body
    request.HTTPBody = [[NSString stringWithUTF8String:content.c_str()] dataUsingEncoding:NSUTF8StringEncoding];
  }

  const auto __weak weakSelf = self;
  NSURLSessionDataTask *task = nil;
  task = [session dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable urlResponse, NSError * _Nullable error) {
    if (!weakSelf) { return; };
    const auto strongSelf = weakSelf;

    const auto response = (NSHTTPURLResponse *)urlResponse;
    std::string json;
    if (data) {
      // Might be no reason to convert to an NSString back to a UTF8 pointer...
      json = std::string([[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding].UTF8String);
    }
    // For some reason I couldn't just do `std::map<std::string, std::string> responseHeaders;` due to std::map's
    // non-const key insertion
    auto* responseHeaders = new std::map<std::string, std::string>();
    [response.allHeaderFields enumerateKeysAndObjectsUsingBlock:^(NSString * _Nonnull key, NSString * _Nonnull obj, BOOL * _Nonnull stop) {
      if (![key isKindOfClass:NSString.class] || ![obj isKindOfClass:NSString.class]) { return; }
      std::string stringKey(key.UTF8String);
      std::string stringValue(obj.UTF8String);
      responseHeaders->insert(std::make_pair(stringKey, stringValue));
    }];
    auto copiedHeaders = std::map<std::string, std::string>(*responseHeaders);
    const auto __weak weakSelf2 = strongSelf;
    dispatch_async(dispatch_get_main_queue(), ^{
      if (!weakSelf2) { return; }
      [weakSelf2.runningTasks removeObject:task];
      callback((int)response.statusCode, json, copiedHeaders);
    });
    delete responseHeaders;
  }];
  // dataTaskWithRequest returns _Nonnull, so no need to worry about initialized variable being nil
  [self.runningTasks addObject:task];
  [task resume];
}

#pragma mark -

- (NSString *)dataPathForFilename:(NSString *)filename
{
  return [self.storagePath stringByAppendingPathComponent:filename];
}

- (bool)saveContents:(const std::string &)contents name:(const std::string &)name
{
  const auto filename = [NSString stringWithUTF8String:name.c_str()];
  const auto nscontents = [NSString stringWithUTF8String:contents.c_str()];
  NSError *error = nil;
  const auto path = [self dataPathForFilename:filename];
  const auto result = [nscontents writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:&error];
  if (error) {
    NSLog(@"Failed to save data for %@: %@", filename, error.localizedDescription);
  }
  return result;
}

- (std::string)loadContentsFromFileWithName:(const std::string &)name
{
  const auto filename = [NSString stringWithUTF8String:name.c_str()];
  NSError *error = nil;
  const auto path = [self dataPathForFilename:filename];
  NSLog(@"Loading contents from file: %@", path);
  const auto contents = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:&error];
  if (error) {
    NSLog(@"Failed to load data for %@: %@", filename, error.localizedDescription);
    return "";
  }
  return std::string(contents.UTF8String);
}

- (bool)removeFileWithName:(const std::string &)name
{
  const auto filename = [NSString stringWithUTF8String:name.c_str()];
  NSError *error = nil;
  const auto path = [self dataPathForFilename:filename];
  const auto result = [NSFileManager.defaultManager removeItemAtPath:path error:&error];
  if (error) {
    NSLog(@"Failed to remove data for filename: %@", filename);
    return false;
  }
  return result;
}

@end
