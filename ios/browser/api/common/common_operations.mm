/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "common_operations.h"
#include <vector>
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"

@interface BraveCommonOperations () {
  scoped_refptr<base::SequencedTaskRunner> _taskRunner;
}
@property(nonatomic, copy) NSString* storagePath;
@property(nonatomic, assign) uint32_t currentTimerID;
@property(nonatomic, copy)
    NSMutableDictionary<NSNumber*, NSTimer*>* timers;  // {ID: Timer}
@property(nonatomic, copy) NSMutableArray<NSURLSessionDataTask*>* runningTasks;
@end

@implementation BraveCommonOperations

- (instancetype)initWithStoragePath:(NSString*)storagePath {
  return
      [self initWithStoragePath:storagePath
                     taskRunner:base::SequencedTaskRunner::GetCurrentDefault()];
}

- (instancetype)initWithStoragePath:(NSString*)storagePath
                         taskRunner:(scoped_refptr<base::SequencedTaskRunner>)
                                        taskRunner {
  if ((self = [super init])) {
    self.storagePath = storagePath;
    _timers = [[NSMutableDictionary alloc] init];
    _runningTasks = [[NSMutableArray alloc] init];
    _taskRunner = taskRunner;

    // Setup the ads directory for persistant storage
    if (self.storagePath.length > 0) {
      if (![NSFileManager.defaultManager fileExistsAtPath:self.storagePath
                                              isDirectory:nil]) {
        [NSFileManager.defaultManager createDirectoryAtPath:self.storagePath
                                withIntermediateDirectories:true
                                                 attributes:nil
                                                      error:nil];
      }
    }
  }
  return self;
}

- (instancetype)init {
  return [self initWithStoragePath:nil];
}

- (void)dealloc {
  [self.runningTasks makeObjectsPerformSelector:@selector(cancel)];
  for (NSNumber* timerID in self.timers) {
    [self.timers[timerID] invalidate];
  }
}

- (const std::string)generateUUID {
  return std::string([NSUUID UUID].UUIDString.UTF8String);
}

- (void)loadURLRequest:(const std::string&)url
               headers:(const std::vector<std::string>&)headers
               content:(const std::string&)content
          content_type:(const std::string&)content_type
                method:(const std::string&)method
              callback:(BATNetworkCompletionBlock)callback {
  const auto session = NSURLSession.sharedSession;
  const auto nsurl =
      [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
  const auto request = [[NSMutableURLRequest alloc] initWithURL:nsurl];

  for (const auto& header : headers) {
    const auto bridged = [NSString stringWithUTF8String:header.c_str()];
    const auto split = [bridged componentsSeparatedByString:@":"];
    if (split.count == 2 && split.firstObject && split.lastObject) {
      auto name = [split.firstObject
          stringByTrimmingCharactersInSet:[NSCharacterSet
                                              whitespaceCharacterSet]];
      auto value = [split.lastObject
          stringByTrimmingCharactersInSet:[NSCharacterSet
                                              whitespaceCharacterSet]];
      [request setValue:value forHTTPHeaderField:name];
    }
  }

  if (self.customUserAgent != nil && self.customUserAgent.length > 0) {
    [request setValue:self.customUserAgent forHTTPHeaderField:@"User-Agent"];
  }

  if (content_type.length() > 0) {
    [request setValue:[NSString stringWithUTF8String:content_type.c_str()]
        forHTTPHeaderField:@"Content-Type"];
  }

  request.HTTPMethod = [NSString stringWithUTF8String:method.c_str()];

  if (method != "GET" && content.length() > 0) {
    // Assumed http body
    request.HTTPBody = [[NSString stringWithUTF8String:content.c_str()]
        dataUsingEncoding:NSUTF8StringEncoding];
  }

  const auto __weak weakSelf = self;
  NSURLSessionDataTask* task = nil;
  task = [session
      dataTaskWithRequest:request
        completionHandler:^(NSData* _Nullable data,
                            NSURLResponse* _Nullable urlResponse,
                            NSError* _Nullable error) {
          if (!weakSelf) {
            return;
          };
          const auto strongSelf = weakSelf;
          strongSelf->_taskRunner->PostTask(
              FROM_HERE, base::BindOnce(^{
                const auto response = (NSHTTPURLResponse*)urlResponse;
                std::string errorDescription;
                if (error) {
                  errorDescription = error.localizedDescription.UTF8String;
                }
                // For some reason I couldn't just do
                // `base::flat_map<std::string, std::string> responseHeaders;`
                // due to base::flat_map's non-const key insertion
                auto* responseHeaders =
                    new base::flat_map<std::string, std::string>();
                [response.allHeaderFields
                    enumerateKeysAndObjectsUsingBlock:^(NSString* _Nonnull key,
                                                        NSString* _Nonnull obj,
                                                        BOOL* _Nonnull stop) {
                      if (![key isKindOfClass:NSString.class] ||
                          ![obj isKindOfClass:NSString.class]) {
                        return;
                      }
                      std::string stringKey(key.UTF8String);
                      std::string stringValue(obj.UTF8String);
                      responseHeaders->insert(
                          std::make_pair(stringKey, stringValue));
                    }];
                auto copiedHeaders =
                    base::flat_map<std::string, std::string>(*responseHeaders);
                [strongSelf.runningTasks removeObject:task];
                callback(errorDescription, (int)response.statusCode, data,
                         copiedHeaders);
                delete responseHeaders;
              }));
        }];
  // dataTaskWithRequest returns _Nonnull, so no need to worry about initialized
  // variable being nil
  [self.runningTasks addObject:task];
  [task resume];
}

#pragma mark -

- (NSString*)dataPathForFilename:(NSString*)filename {
  return [self.storagePath stringByAppendingPathComponent:filename];
}

- (bool)saveContents:(NSData*)contents name:(const std::string&)name {
  if (!contents) {
    return false;
  }

  const auto filename = [NSString stringWithUTF8String:name.c_str()];
  NSError* error = nil;
  const auto path = [self dataPathForFilename:filename];
  const auto result = [contents writeToFile:path
                                    options:NSDataWritingAtomic
                                      error:&error];
  if (error) {
    LOG(ERROR) << "Failed to save data for " << name << ": "
               << base::SysNSStringToUTF8(error.localizedDescription);
  }
  return result;
}

- (std::string)loadContentsFromFileWithName:(const std::string&)name {
  const auto filename = [NSString stringWithUTF8String:name.c_str()];
  NSError* error = nil;
  const auto path = [self dataPathForFilename:filename];
  //  BLOG(2, @"Loading contents from file: %@", path);
  const auto contents = [NSString stringWithContentsOfFile:path
                                                  encoding:NSUTF8StringEncoding
                                                     error:&error];
  if (error) {
    LOG(ERROR) << "Failed to load data for " << name << ": "
               << base::SysNSStringToUTF8(error.localizedDescription);
    return "";
  }
  return std::string(contents.UTF8String);
}

- (bool)removeFileWithName:(const std::string&)name {
  const auto filename = [NSString stringWithUTF8String:name.c_str()];
  NSError* error = nil;
  const auto path = [self dataPathForFilename:filename];
  const auto result = [NSFileManager.defaultManager removeItemAtPath:path
                                                               error:&error];
  if (error) {
    LOG(ERROR) << "Failed to remove data for " << name << ": "
               << base::SysNSStringToUTF8(error.localizedDescription);
    return false;
  }
  return result;
}

@end
