// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/password/importer/brave_password_importer.h"

#include "base/apple/foundation_util.h"
#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/password_manager/core/browser/import/safari_password_importer.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_store/password_store_interface.h"
#include "components/password_manager/core/browser/ui/saved_passwords_presenter.h"
#include "ios/chrome/browser/affiliations/model/ios_chrome_affiliation_service_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_account_password_store_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_profile_password_store_factory.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "ios/chrome/browser/webauthn/model/ios_passkey_model_factory.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"

// MARK: - BravePasswordImportEntryStatus

BravePasswordImportEntryStatus const BravePasswordImportEntryStatusNone =
    static_cast<NSInteger>(password_manager::SafariImportEntry::Status::NONE);
BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusUnknownError = static_cast<NSInteger>(
        password_manager::SafariImportEntry::Status::UNKNOWN_ERROR);
BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusMissingPassword = static_cast<NSInteger>(
        password_manager::SafariImportEntry::Status::MISSING_PASSWORD);
BravePasswordImportEntryStatus const BravePasswordImportEntryStatusMissingURL =
    static_cast<NSInteger>(
        password_manager::SafariImportEntry::Status::MISSING_URL);
BravePasswordImportEntryStatus const BravePasswordImportEntryStatusInvalidURL =
    static_cast<NSInteger>(
        password_manager::SafariImportEntry::Status::INVALID_URL);
BravePasswordImportEntryStatus const BravePasswordImportEntryStatusLongURL =
    static_cast<NSInteger>(
        password_manager::SafariImportEntry::Status::LONG_URL);
BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusLongPassword = static_cast<NSInteger>(
        password_manager::SafariImportEntry::Status::LONG_PASSWORD);
BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusLongUsername = static_cast<NSInteger>(
        password_manager::SafariImportEntry::Status::LONG_USERNAME);
BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusConflictProfile = static_cast<NSInteger>(
        password_manager::SafariImportEntry::Status::CONFLICT_PROFILE);
BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusConflictAccount = static_cast<NSInteger>(
        password_manager::SafariImportEntry::Status::CONFLICT_ACCOUNT);
BravePasswordImportEntryStatus const BravePasswordImportEntryStatusLongNote =
    static_cast<NSInteger>(
        password_manager::SafariImportEntry::Status::LONG_NOTE);
BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusLongConcatenatedNote = static_cast<NSInteger>(
        password_manager::SafariImportEntry::Status::LONG_CONCATENATED_NOTE);
BravePasswordImportEntryStatus const BravePasswordImportEntryStatusValid =
    static_cast<NSInteger>(password_manager::SafariImportEntry::Status::VALID);

// MARK: - BravePasswordImporterResultsStatus

BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusNone = static_cast<NSInteger>(
        password_manager::SafariImportResults::Status::NONE);
BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusUnknownError = static_cast<NSInteger>(
        password_manager::SafariImportResults::Status::UNKNOWN_ERROR);
BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusSuccess = static_cast<NSInteger>(
        password_manager::SafariImportResults::Status::SUCCESS);
BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusIOError = static_cast<NSInteger>(
        password_manager::SafariImportResults::Status::IO_ERROR);
BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusBadFormat = static_cast<NSInteger>(
        password_manager::SafariImportResults::Status::BAD_FORMAT);
BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusDismissed = static_cast<NSInteger>(
        password_manager::SafariImportResults::Status::DISMISSED);
BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusMaxFileSize = static_cast<NSInteger>(
        password_manager::SafariImportResults::Status::MAX_FILE_SIZE);
BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusImportAlreadyActive = static_cast<
        NSInteger>(
        password_manager::SafariImportResults::Status::IMPORT_ALREADY_ACTIVE);
BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusNumPasswordsExceeded = static_cast<
        NSInteger>(
        password_manager::SafariImportResults::Status::NUM_PASSWORDS_EXCEEDED);
BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusConflicts = static_cast<NSInteger>(
        password_manager::SafariImportResults::Status::CONFLICTS);

// MARK: - BravePasswordImportEntry

@implementation BravePasswordImportEntry
- (instancetype)initWithEntry:
    (const password_manager::SafariImportEntry&)entry {
  if ((self = [super init])) {
  }
  return self;
}
@end

// MARK: - BravePasswordImporterResults

@interface BravePasswordImporterResults () {
  password_manager::SafariImportResults results_;
}
@end

@implementation BravePasswordImporterResults
- (instancetype)initWithResults:
    (const password_manager::SafariImportResults&)results {
  if ((self = [super init])) {
    results_ = results;
  }
  return self;
}

- (BravePasswordImporterResultsStatus)status {
  return static_cast<BravePasswordImporterResultsStatus>(results_.status);
}

- (NSString*)fileName {
  return base::SysUTF8ToNSString(results_.file_name);
}

- (NSUInteger)numberImported {
  return results_.number_imported;
}

- (NSArray<BravePasswordImportEntry*>*)displayedEntries {
  NSMutableArray* result = [[NSMutableArray alloc] init];

  for (const auto& entry : results_.displayed_entries) {
    [result addObject:[[BravePasswordImportEntry alloc] initWithEntry:entry]];
  }

  return [result copy];
}
@end

// MARK: - BravePasswordImporter

namespace {
// Returns a passkey model instance if the feature is enabled.
webauthn::PasskeyModel* MaybeGetPasskeyModel(ProfileIOS* profile) {
  return IOSPasskeyModelFactory::GetInstance()->GetForProfile(profile);
}

}  // namespace

@interface BravePasswordImporter () {
  std::unique_ptr<password_manager::SavedPasswordsPresenter> _presenter;
  std::unique_ptr<password_manager::SafariPasswordImporter> _safari_importer;
}
@end

@implementation BravePasswordImporter

- (void)importPasswords:(NSString*)filePath
             completion:(void (^)(BravePasswordImporterResults*))completion {
  __weak BravePasswordImporter* weakSelf = self;

  auto start_import = ^{
    __strong BravePasswordImporter* importer = weakSelf;
    if (!importer) {
      return;
    }

    std::vector<ProfileIOS*> profiles =
        GetApplicationContext()->GetProfileManager()->GetLoadedProfiles();
    ProfileIOS* last_used_profile = profiles.at(0);

    importer->_presenter =
        std::make_unique<password_manager::SavedPasswordsPresenter>(
            IOSChromeAffiliationServiceFactory::GetForProfile(
                last_used_profile),
            IOSChromeProfilePasswordStoreFactory::GetForProfile(
                last_used_profile, ServiceAccessType::EXPLICIT_ACCESS),
            IOSChromeAccountPasswordStoreFactory::GetForProfile(
                last_used_profile, ServiceAccessType::EXPLICIT_ACCESS),
            MaybeGetPasskeyModel(last_used_profile));

    importer->_safari_importer =
        std::make_unique<password_manager::SafariPasswordImporter>(
            importer->_presenter.get());

    // Execute the importer

    importer->_presenter->Init(base::BindOnce(^{
      if (!importer) {
        return;
      }

      base::FilePath path = base::apple::NSStringToFilePath(filePath);
      auto password_store =
          password_manager::PasswordForm::Store::kProfileStore;

      importer->_safari_importer->Import(
          path, password_store,
          base::BindOnce(
              ^(const password_manager::SafariImportResults& results) {
                completion([[BravePasswordImporterResults alloc]
                    initWithResults:results]);
              }));
    }));
  };

  web::GetUIThreadTaskRunner({})->PostTask(FROM_HERE,
                                           base::BindOnce(start_import));
}

- (void)continueImport:(NSArray<BravePasswordImportEntry*>*)entriesToReplace
            completion:(void (^)(BravePasswordImporterResults*))completion {
  __weak BravePasswordImporter* weakSelf = self;

  auto continue_import = ^{
    __strong BravePasswordImporter* importer = weakSelf;
    if (!importer) {
      return;
    }

    std::vector<int> entry_ids;
    for (BravePasswordImportEntry* entry in entriesToReplace) {
      entry_ids.push_back(entry.id);
    }

    importer->_safari_importer->ContinueImport(
        entry_ids,
        base::BindOnce(^(const password_manager::SafariImportResults& results) {
          completion(
              [[BravePasswordImporterResults alloc] initWithResults:results]);
        }));
  };

  web::GetUIThreadTaskRunner({})->PostTask(FROM_HERE,
                                           base::BindOnce(continue_import));
}

@end
