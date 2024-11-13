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

namespace {
// Returns a passkey model instance if the feature is enabled.
webauthn::PasskeyModel* MaybeGetPasskeyModel(ProfileIOS* profile) {
  return IOSPasskeyModelFactory::GetInstance()->GetForProfile(profile);
}

}  // namespace

@interface BravePasswordImporter () {
  std::unique_ptr<password_manager::SavedPasswordsPresenter> _presenter;
  std::unique_ptr<password_manager::SafariPasswordImporter> _importer;
}
@end

@implementation BravePasswordImporter

- (void)importPasswords:(NSString*)filePath completion:(void (^)())completion {
  std::vector<ProfileIOS*> profiles =
      GetApplicationContext()->GetProfileManager()->GetLoadedProfiles();
  ProfileIOS* last_used_profile = profiles.at(0);

  _presenter = std::make_unique<password_manager::SavedPasswordsPresenter>(
      IOSChromeAffiliationServiceFactory::GetForProfile(last_used_profile),
      IOSChromeProfilePasswordStoreFactory::GetForProfile(
          last_used_profile, ServiceAccessType::EXPLICIT_ACCESS),
      IOSChromeAccountPasswordStoreFactory::GetForProfile(
          last_used_profile, ServiceAccessType::EXPLICIT_ACCESS),
      MaybeGetPasskeyModel(last_used_profile));

  base::FilePath path = base::apple::NSStringToFilePath(filePath);
  _importer = std::make_unique<password_manager::SafariPasswordImporter>(
      _presenter.get());

  auto password_store = password_manager::PasswordForm::Store::kProfileStore;

  _importer->Import(
      path, password_store,
      base::BindOnce(
          [](void (^completion)(),
             const password_manager::SafariImportResults& results) {
            completion();
          },
          completion));

  //  _presenter.AddObserver(this);
  _presenter->Init();
}

@end
