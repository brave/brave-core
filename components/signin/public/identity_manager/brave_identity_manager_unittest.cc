/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "components/image_fetcher/core/fake_image_decoder.h"
#include "components/signin/internal/identity_manager/account_fetcher_service.h"
#include "components/signin/internal/identity_manager/account_tracker_service.h"
#include "components/signin/internal/identity_manager/accounts_cookie_mutator_impl.h"
#include "components/signin/internal/identity_manager/accounts_mutator_impl.h"
#include "components/signin/internal/identity_manager/device_accounts_synchronizer_impl.h"
#include "components/signin/internal/identity_manager/diagnostics_provider_impl.h"
#include "components/signin/internal/identity_manager/fake_account_capabilities_fetcher_factory.h"
#include "components/signin/internal/identity_manager/fake_profile_oauth2_token_service.h"
#include "components/signin/internal/identity_manager/gaia_cookie_manager_service.h"
#include "components/signin/internal/identity_manager/primary_account_manager.h"
#include "components/signin/internal/identity_manager/primary_account_mutator_impl.h"
#include "components/signin/public/base/account_consistency_method.h"
#include "components/signin/public/base/list_accounts_test_utils.h"
#include "components/signin/public/base/signin_switches.h"
#include "components/signin/public/base/test_signin_client.h"
#include "components/signin/public/identity_manager/accounts_cookie_mutator.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "components/signin/public/identity_manager/test_identity_manager_observer.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_ANDROID)
#include "components/signin/internal/identity_manager/child_account_info_fetcher_android.h"
#endif

namespace signin {
namespace {

constexpr char kTestGaiaId[] = "dummyId";
constexpr char kTestGaiaId2[] = "dummyId2";
constexpr char kTestEmail[] = "me@gmail.com";
constexpr char kTestEmail2[] = "me2@gmail.com";

}  // namespace

class BraveIdentityManagerTest : public testing::Test {
 protected:
  BraveIdentityManagerTest()
      : signin_client_(&pref_service_, &test_url_loader_factory_) {
    IdentityManager::RegisterProfilePrefs(pref_service_.registry());
    IdentityManager::RegisterLocalStatePrefs(pref_service_.registry());

    RecreateIdentityManager();
  }

  ~BraveIdentityManagerTest() override {
    gaia_cookie_manager_service_ = nullptr;
    identity_manager_->Shutdown();
    signin_client_.Shutdown();
  }

  BraveIdentityManagerTest(const BraveIdentityManagerTest&) = delete;
  BraveIdentityManagerTest& operator=(const BraveIdentityManagerTest&) = delete;

  void SetUp() override {
    primary_account_id_ =
        identity_manager_->PickAccountIdForAccount(kTestGaiaId, kTestEmail);
  }

  IdentityManager* identity_manager() { return identity_manager_.get(); }

  TestIdentityManagerObserver* identity_manager_observer() {
    return identity_manager_observer_.get();
  }

  GaiaCookieManagerService* gaia_cookie_manager_service() {
    return gaia_cookie_manager_service_;
  }

  // Recreates IdentityManager. This process destroys any existing
  // IdentityManager and its dependencies, then remakes them. Dependencies that
  // outlive PrimaryAccountManager (e.g. SigninClient) will be reused.
  void RecreateIdentityManager() {
    // Remove observer first, otherwise IdentityManager destruction might
    // trigger a DCHECK because there are still living observers.
    identity_manager_observer_.reset();

    if (identity_manager_)
      identity_manager_->Shutdown();
    identity_manager_.reset();

    ASSERT_TRUE(temp_profile_dir_.CreateUniqueTempDir());

    auto account_tracker_service = std::make_unique<AccountTrackerService>();
    account_tracker_service->Initialize(&pref_service_,
                                        temp_profile_dir_.GetPath());
    auto token_service =
        std::make_unique<FakeProfileOAuth2TokenService>(&pref_service_);

    auto gaia_cookie_manager_service =
        std::make_unique<GaiaCookieManagerService>(
            account_tracker_service.get(), token_service.get(),
            &signin_client_);
    gaia_cookie_manager_service_ = gaia_cookie_manager_service.get();

    auto account_fetcher_service = std::make_unique<AccountFetcherService>();
    account_fetcher_service->Initialize(
        &signin_client_, token_service.get(), account_tracker_service.get(),
        std::make_unique<image_fetcher::FakeImageDecoder>(),
        std::make_unique<FakeAccountCapabilitiesFetcherFactory>());

    auto primary_account_manager = std::make_unique<PrimaryAccountManager>(
        &signin_client_, token_service.get(), account_tracker_service.get());

    // Passing this switch ensures that the new PrimaryAccountManager starts
    // with a clean slate. Otherwise PrimaryAccountManager::Initialize will use
    // the account id stored in prefs::kGoogleServicesAccountId.
    base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
    cmd_line->AppendSwitch(switches::kClearTokenService);

    IdentityManager::InitParameters init_params;

    init_params.accounts_cookie_mutator =
        std::make_unique<AccountsCookieMutatorImpl>(
            &signin_client_, token_service.get(),
            gaia_cookie_manager_service.get(), account_tracker_service.get());

    init_params.diagnostics_provider =
        std::make_unique<DiagnosticsProviderImpl>(
            token_service.get(), gaia_cookie_manager_service.get());

    init_params.primary_account_mutator =
        std::make_unique<PrimaryAccountMutatorImpl>(
            account_tracker_service.get(), primary_account_manager.get(),
            &pref_service_, &signin_client_);

#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
    init_params.device_accounts_synchronizer =
        std::make_unique<DeviceAccountsSynchronizerImpl>(
            token_service->GetDelegate());
#else
    init_params.accounts_mutator = std::make_unique<AccountsMutatorImpl>(
        token_service.get(), account_tracker_service.get(),
        primary_account_manager.get(), &pref_service_);
#endif

    init_params.account_fetcher_service = std::move(account_fetcher_service);
    init_params.account_tracker_service = std::move(account_tracker_service);
    init_params.gaia_cookie_manager_service =
        std::move(gaia_cookie_manager_service);
    init_params.primary_account_manager = std::move(primary_account_manager);
    init_params.signin_client = &signin_client_;
    init_params.token_service = std::move(token_service);

    identity_manager_ =
        std::make_unique<IdentityManager>(std::move(init_params));
    identity_manager_observer_ =
        std::make_unique<TestIdentityManagerObserver>(identity_manager_.get());
  }

  void TriggerListAccounts() {
    std::vector<gaia::ListedAccount> signed_in_accounts;
    std::vector<gaia::ListedAccount> signed_out_accounts;
    bool accounts_are_fresh = gaia_cookie_manager_service()->ListAccounts(
        &signed_in_accounts, &signed_out_accounts);
    static_cast<void>(accounts_are_fresh);
  }

  const CoreAccountId& primary_account_id() const {
    return primary_account_id_;
  }

  TestSigninClient* signin_client() { return &signin_client_; }

  network::TestURLLoaderFactory* test_url_loader_factory() {
    return &test_url_loader_factory_;
  }

 private:
  base::ScopedTempDir temp_profile_dir_;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  TestSigninClient signin_client_;
  std::unique_ptr<IdentityManager> identity_manager_;
  std::unique_ptr<TestIdentityManagerObserver> identity_manager_observer_;
  CoreAccountId primary_account_id_;
  raw_ptr<GaiaCookieManagerService> gaia_cookie_manager_service_ = nullptr;
};

TEST_F(BraveIdentityManagerTest, GetAccountsInCookieJarWithNoAccounts) {
  base::RunLoop run_loop;
  identity_manager_observer()->SetOnAccountsInCookieUpdatedCallback(
      run_loop.QuitClosure());

  SetListAccountsResponseNoAccounts(test_url_loader_factory());

  TriggerListAccounts();
  const AccountsInCookieJarInfo& accounts_in_cookie_jar =
      identity_manager()->GetAccountsInCookieJar();
  EXPECT_FALSE(accounts_in_cookie_jar.accounts_are_fresh);
  EXPECT_TRUE(accounts_in_cookie_jar.signed_in_accounts.empty());
  EXPECT_TRUE(accounts_in_cookie_jar.signed_out_accounts.empty());

  run_loop.Run();

  TriggerListAccounts();
  const AccountsInCookieJarInfo updated_accounts_in_cookie_jar =
      identity_manager()->GetAccountsInCookieJar();

  EXPECT_FALSE(updated_accounts_in_cookie_jar.accounts_are_fresh);
  EXPECT_TRUE(updated_accounts_in_cookie_jar.signed_in_accounts.empty());
  EXPECT_TRUE(updated_accounts_in_cookie_jar.signed_out_accounts.empty());
}

TEST_F(BraveIdentityManagerTest, GetAccountsInCookieJarWithOneAccount) {
  base::RunLoop run_loop;
  identity_manager_observer()->SetOnAccountsInCookieUpdatedCallback(
      run_loop.QuitClosure());

  SetListAccountsResponseOneAccount(kTestEmail, kTestGaiaId,
                                    test_url_loader_factory());

  TriggerListAccounts();
  const AccountsInCookieJarInfo& accounts_in_cookie_jar =
      identity_manager()->GetAccountsInCookieJar();
  EXPECT_FALSE(accounts_in_cookie_jar.accounts_are_fresh);
  EXPECT_TRUE(accounts_in_cookie_jar.signed_in_accounts.empty());
  EXPECT_TRUE(accounts_in_cookie_jar.signed_out_accounts.empty());

  run_loop.Run();

  TriggerListAccounts();
  const AccountsInCookieJarInfo& updated_accounts_in_cookie_jar =
      identity_manager()->GetAccountsInCookieJar();

  EXPECT_FALSE(updated_accounts_in_cookie_jar.accounts_are_fresh);
  EXPECT_TRUE(updated_accounts_in_cookie_jar.signed_in_accounts.empty());
  EXPECT_TRUE(updated_accounts_in_cookie_jar.signed_out_accounts.empty());
}

TEST_F(BraveIdentityManagerTest, GetAccountsInCookieJarWithTwoAccounts) {
  base::RunLoop run_loop;
  identity_manager_observer()->SetOnAccountsInCookieUpdatedCallback(
      run_loop.QuitClosure());

  SetListAccountsResponseTwoAccounts(kTestEmail, kTestGaiaId, kTestEmail2,
                                     kTestGaiaId2, test_url_loader_factory());

  TriggerListAccounts();
  const AccountsInCookieJarInfo& accounts_in_cookie_jar =
      identity_manager()->GetAccountsInCookieJar();
  EXPECT_FALSE(accounts_in_cookie_jar.accounts_are_fresh);
  EXPECT_TRUE(accounts_in_cookie_jar.signed_in_accounts.empty());
  EXPECT_TRUE(accounts_in_cookie_jar.signed_out_accounts.empty());

  run_loop.Run();

  TriggerListAccounts();
  const AccountsInCookieJarInfo& updated_accounts_in_cookie_jar =
      identity_manager()->GetAccountsInCookieJar();

  EXPECT_FALSE(updated_accounts_in_cookie_jar.accounts_are_fresh);
  EXPECT_TRUE(updated_accounts_in_cookie_jar.signed_in_accounts.empty());
  EXPECT_TRUE(updated_accounts_in_cookie_jar.signed_out_accounts.empty());
}

}  // namespace signin
