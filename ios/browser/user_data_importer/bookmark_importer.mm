// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/user_data_importer/bookmark_importer.h"

#include "base/apple/foundation_util.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"
#include "brave/ios/browser/user_data_importer/bookmark_importer_impl.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_model_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

@implementation BookmarkImporterFactory

+ (id<BookmarkImporter>)getForProfile:(id<ProfileBridge>)profileBridge {
  ProfileIOS* profile =
      base::apple::ObjCCastStrict<ProfileBridgeImpl>(profileBridge).profile;
  return [[BookmarkImporterImpl alloc]
      initWithBookmarkModel:ios::BookmarkModelFactory::GetForProfile(profile)];
}

@end
