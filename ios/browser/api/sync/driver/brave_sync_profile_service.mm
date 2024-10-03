/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/sync/driver/brave_sync_profile_service.h"

#include <unordered_map>

#include "base/memory/raw_ptr.h"
#include "components/sync/base/data_type.h"
#include "components/sync/base/user_selectable_type.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/service/sync_user_settings.h"
#include "ios/web/public/thread/web_thread.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace brave {
namespace ios {
std::unordered_map<syncer::UserSelectableType, BraveSyncUserSelectableTypes>
    mapping = {
        {syncer::UserSelectableType::kBookmarks,
         BraveSyncUserSelectableTypes_BOOKMARKS},
        {syncer::UserSelectableType::kPreferences,
         BraveSyncUserSelectableTypes_PREFERENCES},
        {syncer::UserSelectableType::kPasswords,
         BraveSyncUserSelectableTypes_PASSWORDS},
        {syncer::UserSelectableType::kAutofill,
         BraveSyncUserSelectableTypes_AUTOFILL},
        {syncer::UserSelectableType::kThemes,
         BraveSyncUserSelectableTypes_THEMES},
        {syncer::UserSelectableType::kHistory,
         BraveSyncUserSelectableTypes_HISTORY},
        {syncer::UserSelectableType::kExtensions,
         BraveSyncUserSelectableTypes_EXTENSIONS},
        {syncer::UserSelectableType::kApps, BraveSyncUserSelectableTypes_APPS},
        {syncer::UserSelectableType::kReadingList,
         BraveSyncUserSelectableTypes_READING_LIST},
        {syncer::UserSelectableType::kTabs, BraveSyncUserSelectableTypes_TABS}};

syncer::UserSelectableTypeSet user_types_from_options(
    BraveSyncUserSelectableTypes options) {
  syncer::UserSelectableTypeSet results;
  for (auto it = mapping.begin(); it != mapping.end(); ++it) {
    if (options & it->second) {
      results.Put(it->first);
    }
  }
  return results;
}

BraveSyncUserSelectableTypes options_from_user_types(
    const syncer::UserSelectableTypeSet& types) {
  BraveSyncUserSelectableTypes results = BraveSyncUserSelectableTypes_NONE;
  for (auto it = mapping.begin(); it != mapping.end(); ++it) {
    if (types.Has(it->first)) {
      results |= it->second;
    }
  }
  return results;
}
}  // namespace ios
}  // namespace brave

@interface BraveSyncProfileServiceIOS () {
  raw_ptr<syncer::SyncService> sync_service_;
  std::unordered_map<syncer::UserSelectableType, BraveSyncUserSelectableTypes>
      type_mapping;
}
@end

@implementation BraveSyncProfileServiceIOS

- (instancetype)initWithProfileSyncService:(syncer::SyncService*)syncService {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    sync_service_ = syncService;
  }
  return self;
}

- (bool)isSyncFeatureActive {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  return sync_service_->IsSyncFeatureActive();
}

- (BraveSyncUserSelectableTypes)activeSelectableTypes {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  syncer::DataTypeSet active_types = sync_service_->GetActiveDataTypes();

  syncer::UserSelectableTypeSet user_types;
  for (syncer::UserSelectableType type : syncer::UserSelectableTypeSet::All()) {
    if (active_types.Has(syncer::UserSelectableTypeToCanonicalDataType(type))) {
      user_types.Put(type);
    }
  }
  return brave::ios::options_from_user_types(user_types);
}

- (BraveSyncUserSelectableTypes)userSelectedTypes {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  syncer::UserSelectableTypeSet types =
      sync_service_->GetUserSettings()->GetSelectedTypes();
  return brave::ios::options_from_user_types(types);
}

- (void)setUserSelectedTypes:(BraveSyncUserSelectableTypes)options {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  bool sync_everything = false;
  syncer::UserSelectableTypeSet selected_types =
      brave::ios::user_types_from_options(options);
  sync_service_->GetUserSettings()->SetSelectedTypes(sync_everything,
                                                     selected_types);
}

@end
