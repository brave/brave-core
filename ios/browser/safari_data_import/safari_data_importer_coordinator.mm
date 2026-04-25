// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/safari_data_import/safari_data_importer_coordinator.h"

#include <memory>

#include "base/apple/foundation_util.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"
#include "brave/ios/browser/safari_data_import/safari_data_import_client_bridge.h"
#include "brave/ios/browser/safari_data_import/safari_data_importer_bridge_impl.h"
#include "components/application_locale_storage/application_locale_storage.h"
#include "components/autofill/core/browser/data_manager/personal_data_manager.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/ui/saved_passwords_presenter.h"
#include "components/prefs/pref_service.h"
#include "components/reading_list/core/reading_list_model.h"
#include "components/user_data_importer/ios/ios_bookmark_parser.h"
#include "components/user_data_importer/utility/safari_data_importer.h"
#include "ios/chrome/browser/affiliations/model/ios_chrome_affiliation_service_factory.h"
#include "ios/chrome/browser/autofill/model/personal_data_manager_factory.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_model_factory.h"
#include "ios/chrome/browser/history/model/history_service_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_account_password_store_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_profile_password_store_factory.h"
#include "ios/chrome/browser/reading_list/model/reading_list_model_factory.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/sync/model/sync_service_factory.h"

@implementation SafariDataImporterCoordinatorImpl {
  std::unique_ptr<BraveSafariDataImportClientBridge> _bridge;
  std::unique_ptr<password_manager::SavedPasswordsPresenter>
      _savedPasswordsPresenter;
}

@synthesize importer = _importer;

- (instancetype)initWithProfile:(id<ProfileBridge>)profileBridge {
  if ((self = [super init])) {
    ProfileBridgeImpl* holder =
        base::apple::ObjCCastStrict<ProfileBridgeImpl>(profileBridge);
    /// Use original profile as the user has explicitly requested this operation
    /// to update their personal data.
    ProfileIOS* profile = holder.profile->GetOriginalProfile();
    /// Retrieve dependencies.
    autofill::PersonalDataManager* personalDataManager =
        autofill::PersonalDataManagerFactory::GetForProfile(profile);
    history::HistoryService* historyService =
        ios::HistoryServiceFactory::GetForProfile(
            profile, ServiceAccessType::EXPLICIT_ACCESS);
    bookmarks::BookmarkModel* bookmarkModel =
        ios::BookmarkModelFactory::GetForProfile(profile);
    ReadingListModel* readingListModel =
        ReadingListModelFactory::GetForProfile(profile);
    syncer::SyncService* syncService =
        SyncServiceFactory::GetForProfile(profile);
    PrefService* prefService = profile->GetPrefs();
    std::unique_ptr<user_data_importer::IOSBookmarkParser> bookmarkParser =
        std::make_unique<user_data_importer::IOSBookmarkParser>();
    std::string locale =
        GetApplicationContext()->GetApplicationLocaleStorage()->Get();
    auto* paymentsDataManager = &personalDataManager->payments_data_manager();

    _savedPasswordsPresenter =
        std::make_unique<password_manager::SavedPasswordsPresenter>(
            IOSChromeAffiliationServiceFactory::GetForProfile(profile),
            IOSChromeProfilePasswordStoreFactory::GetForProfile(
                profile, ServiceAccessType::EXPLICIT_ACCESS),
            IOSChromeAccountPasswordStoreFactory::GetForProfile(
                profile, ServiceAccessType::EXPLICIT_ACCESS),
            /*passkey_model=*/nullptr);
    _savedPasswordsPresenter->Init();
    _bridge = std::make_unique<BraveSafariDataImportClientBridge>();
    auto safariDataImporter =
        std::make_unique<user_data_importer::SafariDataImporter>(
            _bridge.get(), _savedPasswordsPresenter.get(), paymentsDataManager,
            historyService, bookmarkModel, readingListModel, syncService,
            prefService, std::move(bookmarkParser), locale);
    _importer = [[SafariDataImporterBridgeImpl alloc]
        initWithSafariDataImporter:std::move(safariDataImporter)];
  }
  return self;
}

- (id<SafariDataImportClientDelegate>)delegate {
  return _bridge->delegate();
}

- (void)setDelegate:(id<SafariDataImportClientDelegate>)delegate {
  _bridge->SetDelegate(delegate);
}

@end
