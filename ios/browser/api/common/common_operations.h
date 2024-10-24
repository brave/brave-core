/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_COMMON_COMMON_OPERATIONS_H_
#define BRAVE_IOS_BROWSER_API_COMMON_COMMON_OPERATIONS_H_

#import <Foundation/Foundation.h>
#include <map>
#include <string>
#include <vector>
#import "base/containers/flat_map.h"
#include "base/task/sequenced_task_runner.h"

NS_ASSUME_NONNULL_BEGIN

/// A standard network completion block. Matches the native-ads/native-rewards
/// signature, but each library uses their own typedef from their namespaces
typedef void (^BATNetworkCompletionBlock)(
    const std::string& errorDescription,
    int statusCode,
    NSData* _Nullable responseData,
    const base::flat_map<std::string, std::string>& headers);

/// A set of common operations that accept and return C++ types
OBJC_EXPORT
@interface BraveCommonOperations : NSObject

- (instancetype)initWithStoragePath:(nullable NSString*)storagePath;
- (instancetype)initWithStoragePath:(nullable NSString*)storagePath
                         taskRunner:(scoped_refptr<base::SequencedTaskRunner>)
                                        taskRunner NS_DESIGNATED_INITIALIZER;

#pragma mark -

/// Generates a UUID using NSUUID
- (const std::string)generateUUID;

#pragma mark - Network

@property(nonatomic, copy, nullable) NSString* customUserAgent;

/// Loads a URL request
- (void)loadURLRequest:(const std::string&)url
               headers:(const std::vector<std::string>&)headers
               content:(const std::string&)content
          content_type:(const std::string&)content_type
                method:(const std::string&)method
              callback:(BATNetworkCompletionBlock)callback;

#pragma mark - File Managment

/// Retuns the path to a file with the given name
- (NSString*)dataPathForFilename:(NSString*)filename;

/// Save the contents to a file with the given name
- (bool)saveContents:(NSData* _Nullable)contents name:(const std::string&)name;
/// Load the contents of a saved file with the given name
- (std::string)loadContentsFromFileWithName:(const std::string&)name;
/// Remove the saved file with the given name
- (bool)removeFileWithName:(const std::string&)name;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_COMMON_COMMON_OPERATIONS_H_
