/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#import <string>
#import <map>

NS_ASSUME_NONNULL_BEGIN

/// A standard network completion block. Matches the native-ads/native-rewards signature, but
/// each library uses their own typedef from their namespaces
typedef void (^BATNetworkCompletionBlock)(int statusCode,
                                          const std::string& response,
                                          const std::map<std::string, std::string>& headers);

/// A set of common operations that accept and return C++ types
@interface BATCommonOperations : NSObject

- (instancetype)initWithStoragePath:(nullable NSString *)storagePath NS_DESIGNATED_INITIALIZER;

#pragma mark -

/// Generates a UUID using NSUUID
- (const std::string)generateUUID;

#pragma mark - Timers

/// Creates a timer that will fire after `offset`. When the timer fires `timerFired` is executed with the timer ID
/// returned
- (uint32_t)createTimerWithOffset:(uint64_t)offset timerFired:(void (^)(uint32_t))timerFired;

/// Invalidates and removes the timer with the given timer ID
- (void)removeTimerWithID:(uint32_t)timerID;

#pragma mark - Network

/// Loads a URL request
- (void)loadURLRequest:(const std::string&)url
               headers:(const std::vector<std::string>&)headers
               content:(const std::string&)content
          content_type:(const std::string&)content_type
                method:(const std::string&)method
              callback:(BATNetworkCompletionBlock)callback;

#pragma mark - File Managment

/// Save the contents to a file with the given name
- (bool)saveContents:(const std::string&)contents name:(const std::string&)name;
/// Load the contents of a saved file with the given name
- (std::string)loadContentsFromFileWithName:(const std::string&)name;
/// Remove the saved file with the given name
- (bool)removeFileWithName:(const std::string&)name;

@end

NS_ASSUME_NONNULL_END
