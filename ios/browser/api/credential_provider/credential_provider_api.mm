/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/credential_provider/credential_provider_api.h"

#include "ios/chrome/common/app_group/app_group_constants.h"
#include "ios/chrome/common/credential_provider/archivable_credential_store.h"
#include "ios/chrome/common/credential_provider/constants.h"
#include "ios/chrome/common/credential_provider/credential.h"
#include "ios/chrome/common/credential_provider/credential_store.h"
#include "ios/chrome/common/credential_provider/multi_store_credential_store.h"
#include "ios/chrome/common/credential_provider/user_defaults_credential_store.h"

@implementation CredentialProviderAPI

// matches credential_provider_view_controller.mm
+ (id<CredentialStore>)credentialStore {
  ArchivableCredentialStore* archivableStore =
      [[ArchivableCredentialStore alloc]
          initWithFileURL:CredentialProviderSharedArchivableStoreURL()];

  NSString* key = AppGroupUserDefaultsCredentialProviderNewCredentials();
  UserDefaultsCredentialStore* defaultsStore =
      [[UserDefaultsCredentialStore alloc]
          initWithUserDefaults:app_group::GetGroupUserDefaults()
                           key:key];
  return [[MultiStoreCredentialStore alloc]
      initWithStores:@[ defaultsStore, archivableStore ]];
}

// mostly matches credential_list_view_controller.mm
+ (void)loadAttributesForCredential:(id<Credential>)credential
                         completion:(void (^)(FaviconAttributes* _Nullable))
                                        completion {
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
    NSURL* attributesFolder = app_group::SharedFaviconAttributesFolder();
    if (!attributesFolder) {
      dispatch_async(dispatch_get_main_queue(), ^{
        completion(nil);
      });
      return;
    }
    NSURL* filePath =
        [attributesFolder URLByAppendingPathComponent:credential.favicon
                                          isDirectory:NO];
    NSError* error = nil;
    NSData* data = [NSData dataWithContentsOfURL:filePath
                                         options:0
                                           error:&error];
    if (data && !error) {
      NSKeyedUnarchiver* unarchiver =
          [[NSKeyedUnarchiver alloc] initForReadingFromData:data error:nil];
      unarchiver.requiresSecureCoding = NO;
      FaviconAttributes* attributes =
          [unarchiver decodeObjectForKey:NSKeyedArchiveRootObjectKey];
      dispatch_async(dispatch_get_main_queue(), ^{
        completion(attributes);
      });
    } else {
      dispatch_async(dispatch_get_main_queue(), ^{
        completion(nil);
      });
    }
  });
}

@end
