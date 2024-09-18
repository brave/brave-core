/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ethereum_provider_impl.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_service_delegate_impl.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_tx_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/permissions/brave_permission_manager.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "brave/components/version_info/version_info.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/permissions/permission_manager_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/permission_request_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_contents_factory.h"
#include "content/test/test_web_contents.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

using testing::ElementsAre;
using testing::IsEmpty;

namespace brave_wallet {

namespace {

void GetErrorCodeMessage(base::Value formed_response,
                         mojom::ProviderError* error,
                         std::string* error_message) {
  if (!formed_response.is_dict()) {
    *error = mojom::ProviderError::kSuccess;
    error_message->clear();
    return;
  }
  auto code = formed_response.GetDict().FindInt("code");
  if (code) {
    *error = static_cast<mojom::ProviderError>(*code);
  }
  const std::string* message = formed_response.GetDict().FindString("message");
  if (message) {
    *error_message = *message;
  }
}

void ValidateErrorCode(EthereumProviderImpl* provider,
                       const std::string& payload,
                       mojom::ProviderError expected) {
  bool callback_is_called = false;
  provider->AddEthereumChain(
      payload,
      base::BindLambdaForTesting(
          [&callback_is_called, &expected](
              base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, expected);
            ASSERT_FALSE(error_message.empty());
            callback_is_called = true;
          }),
      base::Value());
  ASSERT_TRUE(callback_is_called);
}

std::vector<uint8_t> DecodeHexHash(const std::string& hash_hex) {
  std::vector<uint8_t> hash;
  base::HexStringToBytes(hash_hex, &hash);
  return hash;
}

std::optional<base::Value> ToValue(const network::ResourceRequest& request) {
  std::string_view request_string(request.request_body->elements()
                                      ->at(0)
                                      .As<network::DataElementBytes>()
                                      .AsStringPiece());
  return base::JSONReader::Read(request_string,
                                base::JSONParserOptions::JSON_PARSE_RFC);
}

class TestEventsListener : public brave_wallet::mojom::EventsListener {
 public:
  TestEventsListener() = default;

  MOCK_METHOD1(ChainChangedEvent, void(const std::string&));

  void AccountsChangedEvent(const std::vector<std::string>& accounts) override {
    lowercase_accounts_.resize(accounts.size());
    std::transform(accounts.cbegin(), accounts.cend(),
                   lowercase_accounts_.begin(),
                   [](auto&& account) { return base::ToLowerASCII(account); });
    accounts_changed_fired_ = true;
  }

  void MessageEvent(const std::string& subscription_id,
                    base::Value result) override {
    message_event_fired_ = true;
    last_message_ = std::move(result);
  }

  bool AccountsChangedFired() const {
    base::RunLoop().RunUntilIdle();
    return accounts_changed_fired_;
  }

  bool MessageEventFired() const {
    base::RunLoop().RunUntilIdle();
    return message_event_fired_;
  }

  base::Value GetLastMessage() const {
    base::RunLoop().RunUntilIdle();
    return last_message_.Clone();
  }

  std::vector<std::string> GetLowercaseAccounts() const {
    base::RunLoop().RunUntilIdle();
    return lowercase_accounts_;
  }

  mojo::PendingRemote<brave_wallet::mojom::EventsListener> GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  void Reset() {
    lowercase_accounts_.clear();
    accounts_changed_fired_ = false;
    message_event_fired_ = false;
    EXPECT_FALSE(AccountsChangedFired());
    EXPECT_FALSE(MessageEventFired());
  }

  bool accounts_changed_fired_ = false;
  bool message_event_fired_ = false;
  std::vector<std::string> lowercase_accounts_;
  base::Value last_message_;

 private:
  mojo::Receiver<brave_wallet::mojom::EventsListener> observer_receiver_{this};
};

}  // namespace

class EthereumProviderImplUnitTest : public testing::Test {
 public:
  EthereumProviderImplUnitTest()
      : browser_task_environment_(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void TearDown() override {
    provider_.reset();
    web_contents_.reset();
    profile_.SetPermissionControllerDelegate(nullptr);
    BraveWalletServiceDelegateImpl::SetActiveWebContentsForTesting(nullptr);
  }

  void SetUp() override {
    // Resetting this test callback, as it gets stored in a discreet global, and
    // in some cases it was causing stack-use-after-return.
    SetCallbackForNewSetupNeededForTesting(base::OnceCallback<void()>());

    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());
    web_contents_ =
        content::TestWebContents::Create(browser_context(), nullptr);
    BraveWalletServiceDelegateImpl::SetActiveWebContentsForTesting(
        web_contents_.get());
    permissions::PermissionRequestManager::CreateForWebContents(web_contents());
    asset_ratio_service_ =
        AssetRatioServiceFactory::GetServiceForContext(browser_context());
    asset_ratio_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
    brave_wallet_service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_,
        BraveWalletServiceDelegate::Create(browser_context()), prefs(),
        local_state_->Get());
    ASSERT_TRUE(brave_wallet_service_.get());
    json_rpc_service_ = brave_wallet_service_->json_rpc_service();
    json_rpc_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
    SetNetwork(mojom::kMainnetChainId, std::nullopt);
    keyring_service_ = brave_wallet_service_->keyring_service();
    tx_service_ = brave_wallet_service_->tx_service();
    WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());
    SetNetwork(mojom::kMainnetChainId, std::nullopt);

    profile_.SetPermissionControllerDelegate(
        base::WrapUnique(static_cast<permissions::BravePermissionManager*>(
            PermissionManagerFactory::GetInstance()
                ->BuildServiceInstanceForBrowserContext(browser_context())
                .release())));

    provider_ = std::make_unique<EthereumProviderImpl>(
        host_content_settings_map(), brave_wallet_service_.get(),
        std::make_unique<brave_wallet::BraveWalletProviderDelegateImpl>(
            web_contents(), web_contents()->GetPrimaryMainFrame()),
        prefs());

    observer_ = std::make_unique<TestEventsListener>();
    provider_->Init(observer_->GetReceiver());
  }

  EthTxManager* eth_tx_manager() {
    return static_cast<EthTxManager*>(
        tx_service_->GetTxManager(mojom::CoinType::ETH));
  }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetNetwork(const std::string& chain_id,
                  const std::optional<::url::Origin>& origin) {
    EXPECT_TRUE(
        json_rpc_service_->SetNetwork(chain_id, mojom::CoinType::ETH, origin));
  }

  void CreateWallet() {
    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
  }

  mojom::AccountInfoPtr AddHardwareAccount(const std::string& address) {
    std::vector<mojom::HardwareWalletAccountPtr> hw_accounts;
    hw_accounts.push_back(mojom::HardwareWalletAccount::New(
        address, "m/44'/60'/1'/0/0", "name 1", mojom::HardwareVendor::kLedger,
        "device1", mojom::kDefaultKeyringId));

    auto added_accounts =
        keyring_service_->AddHardwareAccountsSync(std::move(hw_accounts));
    return std::move(added_accounts[0]);
  }

  void Unlock() {
    base::RunLoop run_loop;
    keyring_service_->Unlock(
        "brave", base::BindLambdaForTesting([&run_loop](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void Lock() {
    keyring_service_->Lock();
    browser_task_environment_.RunUntilIdle();
  }

  void SetSelectedAccount(const mojom::AccountIdPtr& account_id) {
    base::RunLoop run_loop;
    keyring_service_->SetSelectedAccount(
        account_id.Clone(), base::BindLambdaForTesting([&](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  std::vector<std::string> GetAllowedAccounts(
      bool include_accounts_when_locked) {
    const auto allowed_accounts =
        provider()->GetAllowedAccounts(include_accounts_when_locked);
    EXPECT_TRUE(allowed_accounts);
    return *allowed_accounts;
  }

  std::pair<bool, base::Value> CommonRequestOrSendAsync(
      base::ValueView input_value,
      bool format_json_rpc_response = false) {
    base::RunLoop run_loop;
    std::pair<bool, base::Value> response;
    provider()->CommonRequestOrSendAsync(
        input_value,
        base::BindLambdaForTesting(
            [&](base::Value id, base::Value formed_response, const bool reject,
                const std::string& first_allowed_account,
                const bool update_bind_js_properties) {
              response = std::make_pair(reject, std::move(formed_response));
              run_loop.Quit();
            }),
        format_json_rpc_response);
    run_loop.Run();
    return response;
  }

  std::vector<std::string> RequestEthereumPermissions() {
    std::vector<std::string> allowed_accounts;
    base::RunLoop run_loop;
    provider()->RequestEthereumPermissions(
        base::BindLambdaForTesting(
            [&](base::Value id, base::Value formed_response, const bool reject,
                const std::string& first_allowed_account,
                const bool update_bind_js_properties) {
              if (formed_response.GetList().size() != 0) {
                std::string stylesheet = "";
                for (auto& account : formed_response.GetList()) {
                  allowed_accounts.push_back(account.GetString());
                }
              }
              run_loop.Quit();
            }),
        base::Value(), "", GetOrigin());
    run_loop.Run();
    return allowed_accounts;
  }

  EthereumProviderImpl::RequestCallback CreateResponseCallback(
      base::RunLoop& run_loop,
      std::pair<bool, base::Value>& response) {
    return base::BindLambdaForTesting(
        [&run_loop, &response](base::Value id, base::Value formed_response,
                               const bool reject,
                               const std::string& first_allowed_account,
                               const bool update_bind_js_properties) {
          response = std::make_pair(reject, std::move(formed_response));
          run_loop.Quit();
        });
  }

  std::pair<bool, base::Value> Enable() {
    base::RunLoop run_loop;
    std::pair<bool, base::Value> response;
    provider()->Enable(CreateResponseCallback(run_loop, response));
    run_loop.Run();
    return response;
  }

  std::pair<bool, base::Value> Request(base::Value input) {
    base::RunLoop run_loop;
    std::pair<bool, base::Value> response;
    provider()->Request(std::move(input),
                        CreateResponseCallback(run_loop, response));
    run_loop.Run();
    return response;
  }

  std::pair<bool, base::Value> Send(const std::string& method,
                                    base::Value params) {
    base::RunLoop run_loop;
    std::pair<bool, base::Value> response;
    provider()->Send(method, std::move(params),
                     CreateResponseCallback(run_loop, response));
    run_loop.Run();
    return response;
  }

  std::pair<bool, base::Value> SendAsync(base::Value input) {
    base::RunLoop run_loop;
    std::pair<bool, base::Value> response;
    provider()->SendAsync(std::move(input),
                          CreateResponseCallback(run_loop, response));
    run_loop.Run();
    return response;
  }

  ~EthereumProviderImplUnitTest() override = default;

  content::TestWebContents* web_contents() { return web_contents_.get(); }
  TxService* tx_service() { return tx_service_; }
  JsonRpcService* json_rpc_service() { return json_rpc_service_; }
  KeyringService* keyring_service() { return keyring_service_; }
  AccountUtils GetAccountUtils() { return AccountUtils(keyring_service_); }
  EthereumProviderImpl* provider() { return provider_.get(); }
  content::BrowserContext* browser_context() { return &profile_; }
  PrefService* prefs() { return profile_.GetPrefs(); }
  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(&profile_);
  }

  void Navigate(const GURL& url) { web_contents()->NavigateAndCommit(url); }

  url::Origin GetOrigin() {
    return web_contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin();
  }

  void CreateBraveWalletTabHelper() {
    brave_wallet::BraveWalletTabHelper::CreateForWebContents(
        web_contents_.get());
  }
  brave_wallet::BraveWalletTabHelper* brave_wallet_tab_helper() {
    return brave_wallet::BraveWalletTabHelper::FromWebContents(
        web_contents_.get());
  }

  void AddEthereumPermission(const mojom::AccountIdPtr& account_id) {
    EXPECT_TRUE(permissions::BraveWalletPermissionContext::AddPermission(
        blink::PermissionType::BRAVE_ETHEREUM, browser_context(), GetOrigin(),
        account_id->address));
  }

  void ResetEthereumPermission(const mojom::AccountIdPtr& account_id) {
    base::RunLoop run_loop;
    brave_wallet_service_->ResetPermission(
        account_id.Clone(), base::BindLambdaForTesting([&](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void Web3ClientVersion(std::string* version,
                         mojom::ProviderError* error_out,
                         std::string* error_message_out) {
    if (!version || !error_out || !error_message_out) {
      return;
    }
    base::RunLoop run_loop;
    provider()->Web3ClientVersion(
        base::BindLambdaForTesting(
            [&](base::Value id, base::Value formed_response, const bool reject,
                const std::string& first_allowed_account,
                const bool update_bind_js_properties) {
              if (formed_response.type() == base::Value::Type::STRING) {
                *version = formed_response.GetString();
              }
              GetErrorCodeMessage(std::move(formed_response), error_out,
                                  error_message_out);
              run_loop.Quit();
            }),
        base::Value());
    run_loop.Run();
  }

  std::string GetSIWEMessage(const std::string& domain,
                             const std::string& account,
                             const std::string& uri,
                             const std::string& network) {
    return base::StringPrintf(
        "%s wants you to sign in with your Ethereum account:\n"
        "%s\n\n\n"
        "URI: %s\n"
        "Version: 1\n"
        "Chain ID: %s\n"
        "Nonce: 32891756\n"
        "Issued At: 2021-09-30T16:25:24Z)",
        domain.c_str(), account.c_str(), uri.c_str(), network.c_str());
  }

  void SignMessageHardware(
      bool user_approved,
      const std::string& address,
      const std::string& message,
      const mojom::EthereumSignatureBytesPtr& hardware_signature,
      const std::string& error_in,
      mojom::EthereumSignatureBytesPtr* signature_out,
      mojom::ProviderError* error_out,
      std::string* error_message_out) {
    if (!signature_out || !error_out || !error_message_out) {
      return;
    }

    base::RunLoop run_loop;
    provider()->SignMessage(
        address, message,
        base::BindLambdaForTesting(
            [&](base::Value id, base::Value formed_response, const bool reject,
                const std::string& first_allowed_account,
                const bool update_bind_js_properties) {
              signature_out->reset();
              if (formed_response.type() == base::Value::Type::STRING) {
                *signature_out = mojom::EthereumSignatureBytes::New(
                    *PrefixedHexStringToBytes(formed_response.GetString()));
              }
              GetErrorCodeMessage(std::move(formed_response), error_out,
                                  error_message_out);
              run_loop.Quit();
            }),
        base::Value());
    // Wait for EthereumProviderImpl::ContinueSignMessage
    browser_task_environment_.RunUntilIdle();
    brave_wallet_service_->NotifySignMessageRequestProcessed(
        user_approved, brave_wallet_service_->sign_message_id_ - 1,
        hardware_signature.Clone(), error_in);
    run_loop.Run();
  }

  void SignMessage(std::optional<bool> user_approved,
                   const std::string& address,
                   const std::string& message,
                   std::string* signature_out,
                   mojom::ProviderError* error_out,
                   std::string* error_message_out) {
    if (!signature_out || !error_out || !error_message_out) {
      return;
    }

    base::RunLoop run_loop;
    provider()->SignMessage(
        address, message,
        base::BindLambdaForTesting(
            [&](base::Value id, base::Value formed_response, const bool reject,
                const std::string& first_allowed_account,
                const bool update_bind_js_properties) {
              signature_out->clear();
              if (formed_response.type() == base::Value::Type::STRING) {
                *signature_out = formed_response.GetString();
              }
              GetErrorCodeMessage(std::move(formed_response), error_out,
                                  error_message_out);
              run_loop.Quit();
            }),
        base::Value());
    // Wait for EthereumProviderImpl::ContinueSignMessage
    browser_task_environment_.RunUntilIdle();
    if (user_approved) {
      brave_wallet_service_->NotifySignMessageRequestProcessed(
          *user_approved, brave_wallet_service_->sign_message_id_ - 1, nullptr,
          std::nullopt);
    }
    run_loop.Run();
  }

  void RecoverAddress(const std::string& message,
                      const std::string& signature,
                      std::string* address_out,
                      mojom::ProviderError* error_out,
                      std::string* error_message_out) {
    if (!address_out || !error_out || !error_message_out) {
      return;
    }

    base::RunLoop run_loop;
    provider()->RecoverAddress(
        message, signature,
        base::BindLambdaForTesting(
            [&](base::Value id, base::Value formed_response, const bool reject,
                const std::string& first_allowed_account,
                const bool update_bind_js_properties) {
              address_out->clear();
              if (formed_response.type() == base::Value::Type::STRING) {
                *address_out = formed_response.GetString();
              }
              GetErrorCodeMessage(std::move(formed_response), error_out,
                                  error_message_out);
              run_loop.Quit();
            }),
        base::Value());
    run_loop.Run();
  }

  void SignTypedMessage(std::optional<bool> user_approved,
                        const std::string& address,
                        const std::string& message,
                        const std::vector<uint8_t>& domain_hash,
                        const std::vector<uint8_t>& primary_hash,
                        base::Value::Dict domain,
                        mojom::EthSignTypedDataMetaPtr meta,
                        std::string* signature_out,
                        mojom::ProviderError* error_out,
                        std::string* error_message_out) {
    if (!signature_out || !error_out || !error_message_out) {
      return;
    }

    base::RunLoop run_loop;
    provider()->SignTypedMessage(
        address, message, domain_hash, primary_hash, std::move(meta),
        std::move(domain),
        base::BindLambdaForTesting(
            [&](base::Value id, base::Value formed_response, const bool reject,
                const std::string& first_allowed_account,
                const bool update_bind_js_properties) {
              signature_out->clear();
              if (formed_response.type() == base::Value::Type::STRING) {
                *signature_out = formed_response.GetString();
              }
              GetErrorCodeMessage(std::move(formed_response), error_out,
                                  error_message_out);
              run_loop.Quit();
            }),
        base::Value());
    // Wait for EthereumProviderImpl::ContinueSignMessage
    browser_task_environment_.RunUntilIdle();
    if (user_approved) {
      brave_wallet_service_->NotifySignMessageRequestProcessed(
          *user_approved, brave_wallet_service_->sign_message_id_ - 1, nullptr,
          std::nullopt);
    }
    run_loop.Run();
  }

  // current request id will be returned
  int SignMessageRequest(const std::string& address,
                         const std::string& message) {
    provider()->SignMessage(address, message, base::DoNothing(), base::Value());
    browser_task_environment_.RunUntilIdle();
    return brave_wallet_service_->sign_message_id_ - 1;
  }

  size_t GetSignMessageQueueSize() const {
    size_t request_queue_size =
        brave_wallet_service_->sign_message_requests_.size();
    EXPECT_EQ(brave_wallet_service_->sign_message_callbacks_.size(),
              request_queue_size);
    return request_queue_size;
  }

  const mojom::SignMessageRequestPtr& GetSignMessageQueueFront() const {
    return brave_wallet_service_->sign_message_requests_.front();
  }
  const mojom::SignMessageErrorPtr& GetSignMessageErrorQueueFront() const {
    return brave_wallet_service_->sign_message_errors_.front();
  }

  std::vector<mojom::SignMessageRequestPtr> GetPendingSignMessageRequests() {
    base::RunLoop run_loop;
    std::vector<mojom::SignMessageRequestPtr> requests_out;
    brave_wallet_service_->GetPendingSignMessageRequests(
        base::BindLambdaForTesting(
            [&](std::vector<mojom::SignMessageRequestPtr> requests) {
              for (const auto& request : requests) {
                ASSERT_TRUE(request->sign_data->is_eth_standard_sign_data());
                SCOPED_TRACE(
                    request->sign_data->get_eth_standard_sign_data()->message);
                EXPECT_EQ(request->chain_id,
                          json_rpc_service_->GetChainIdSync(
                              mojom::CoinType::ETH, GetOrigin()));
                requests_out.push_back(request.Clone());
              }
              run_loop.Quit();
            }));
    run_loop.Run();
    return requests_out;
  }

  std::vector<mojom::GetEncryptionPublicKeyRequestPtr>
  GetPendingGetEncryptionPublicKeyRequests() const {
    base::RunLoop run_loop;
    std::vector<mojom::GetEncryptionPublicKeyRequestPtr> requests_out;
    brave_wallet_service_->GetPendingGetEncryptionPublicKeyRequests(
        base::BindLambdaForTesting(
            [&](std::vector<mojom::GetEncryptionPublicKeyRequestPtr> requests) {
              for (const auto& request : requests) {
                requests_out.push_back(request.Clone());
              }
              run_loop.Quit();
            }));
    run_loop.Run();
    return requests_out;
  }

  std::vector<mojom::DecryptRequestPtr> GetPendingDecryptRequests() const {
    base::RunLoop run_loop;
    std::vector<mojom::DecryptRequestPtr> requests_out;
    brave_wallet_service_->GetPendingDecryptRequests(base::BindLambdaForTesting(
        [&](std::vector<mojom::DecryptRequestPtr> requests) {
          for (const auto& request : requests) {
            requests_out.push_back(request.Clone());
          }
          run_loop.Quit();
        }));
    run_loop.Run();
    return requests_out;
  }

  std::vector<std::string> GetAddresses() {
    std::vector<std::string> result;
    for (const auto& account_info : keyring_service_->GetAllAccountInfos()) {
      if (account_info->account_id->coin == mojom::CoinType::ETH) {
        result.push_back(account_info->address);
      }
    }
    return result;
  }

  mojom::TransactionInfoPtr GetTransactionInfo(const std::string& meta_id) {
    mojom::TransactionInfoPtr transaction_info;
    base::RunLoop run_loop;
    tx_service()->GetTransactionInfo(
        mojom::CoinType::ETH, meta_id,
        base::BindLambdaForTesting([&](mojom::TransactionInfoPtr v) {
          transaction_info = std::move(v);
          run_loop.Quit();
        }));
    run_loop.Run();
    return transaction_info;
  }

  std::vector<mojom::TransactionInfoPtr> GetAllTransactionInfo(
      const mojom::AccountIdPtr& account_id,
      const std::string& chain_id) {
    std::vector<mojom::TransactionInfoPtr> transaction_infos;
    base::RunLoop run_loop;
    tx_service()->GetAllTransactionInfo(
        mojom::CoinType::ETH, chain_id, account_id.Clone(),
        base::BindLambdaForTesting(
            [&](std::vector<mojom::TransactionInfoPtr> v) {
              transaction_infos = std::move(v);
              run_loop.Quit();
            }));
    run_loop.Run();
    return transaction_infos;
  }

  bool ApproveTransaction(const std::string& chain_id,
                          const std::string& tx_meta_id,
                          mojom::ProviderError* error_out,
                          std::string* error_message_out) {
    bool success;
    base::RunLoop run_loop;
    tx_service()->ApproveTransaction(
        mojom::CoinType::ETH, chain_id, tx_meta_id,
        base::BindLambdaForTesting([&](bool v,
                                       mojom::ProviderErrorUnionPtr error,
                                       const std::string& error_message) {
          ASSERT_TRUE(error->is_provider_error());
          success = v;
          *error_out = error->get_provider_error();
          *error_message_out = error_message;
          run_loop.Quit();
        }));
    run_loop.Run();
    return success;
  }

  void SwitchEthereumChain(const std::string& chain_id,
                           std::optional<bool> user_approved,
                           mojom::ProviderError* error_out,
                           std::string* error_message_out) {
    base::RunLoop run_loop;
    provider_->SwitchEthereumChain(
        chain_id,
        base::BindLambdaForTesting(
            [&](base::Value id, base::Value formed_response, const bool reject,
                const std::string& first_allowed_account,
                const bool update_bind_js_properties) {
              GetErrorCodeMessage(std::move(formed_response), error_out,
                                  error_message_out);
              run_loop.Quit();
            }),
        base::Value());
    if (user_approved) {
      json_rpc_service_->NotifySwitchChainRequestProcessed(
          GetPendingSwitchChainRequestId(), *user_approved);
    }
    run_loop.Run();
  }

  void GetEncryptionPublicKey(const std::string& address,
                              bool approved,
                              std::string* key_out,
                              mojom::ProviderError* error_out,
                              std::string* error_message_out) {
    base::RunLoop run_loop;
    provider_->GetEncryptionPublicKey(
        address,
        base::BindLambdaForTesting([&](base::Value id,
                                       base::Value formed_response, bool reject,
                                       const std::string& first_allowed_account,
                                       const bool update_bind_js_properties) {
          *key_out = "";
          if (formed_response.type() == base::Value::Type::STRING) {
            *key_out = formed_response.GetString();
          }
          mojom::ProviderError error = mojom::ProviderError::kUnknown;
          std::string error_message;
          GetErrorCodeMessage(std::move(formed_response), &error,
                              &error_message);
          *error_out = error;
          *error_message_out = error_message;
          run_loop.Quit();
        }),
        base::Value());
    // Wait for KeyringService::GetSelectedAccount called by
    // BraveWalletProviderDelegateImpl::GetAllowedAccounts
    browser_task_environment_.RunUntilIdle();
    auto requests = GetPendingGetEncryptionPublicKeyRequests();
    if (requests.size() > 0) {
      ASSERT_EQ(requests.size(), 1u);
      EXPECT_EQ(requests[0]->origin_info, MakeOriginInfo(GetOrigin()));
      EXPECT_EQ(requests[0]->account_id,
                GetAccountUtils().FindAccountIdByAddress(address));
      EXPECT_TRUE(brave_wallet_tab_helper()->IsShowingBubble());
      brave_wallet_service_->NotifyGetPublicKeyRequestProcessed(
          requests[0]->request_id, approved);
    }
    run_loop.Run();
  }

  void Decrypt(const std::string& encrypted_data_json,
               const std::string& address,
               bool approved,
               std::string* unsafe_message,
               mojom::ProviderError* error_out,
               std::string* error_message_out) {
    *unsafe_message = "";
    base::RunLoop run_loop;
    provider_->Decrypt(
        encrypted_data_json, address, GetOrigin(),
        base::BindLambdaForTesting([&](base::Value id,
                                       base::Value formed_response, bool reject,
                                       const std::string& first_allowed_account,
                                       const bool update_bind_js_properties) {
          if (formed_response.type() == base::Value::Type::STRING) {
            *unsafe_message = formed_response.GetString();
          }
          mojom::ProviderError error = mojom::ProviderError::kUnknown;
          std::string error_message;
          GetErrorCodeMessage(std::move(formed_response), &error,
                              &error_message);
          *error_out = error;
          *error_message_out = error_message;
          run_loop.Quit();
        }),
        base::Value());
    // The request is not immediately added, needs sanitization first
    browser_task_environment_.RunUntilIdle();
    auto requests = GetPendingDecryptRequests();
    if (requests.size() > 0) {
      ASSERT_EQ(requests.size(), 1u);
      EXPECT_EQ(requests[0]->origin_info, MakeOriginInfo(GetOrigin()));
      EXPECT_EQ(requests[0]->account_id,
                GetAccountUtils().FindAccountIdByAddress(address));
      EXPECT_TRUE(brave_wallet_tab_helper()->IsShowingBubble());
      brave_wallet_service_->NotifyDecryptRequestProcessed(
          requests[0]->request_id, approved);
    }
    run_loop.Run();
  }

  void AddSuggestToken(mojom::BlockchainTokenPtr token,
                       bool approved,
                       bool* approved_out,
                       mojom::ProviderError* error_out,
                       std::string* error_message_out) {
    base::RunLoop run_loop;
    provider_->AddSuggestToken(
        token.Clone(),
        base::BindLambdaForTesting(
            [&](base::Value id, base::Value formed_response, const bool reject,
                const std::string& first_allowed_account,
                const bool update_bind_js_properties) {
              if (formed_response.type() == base::Value::Type::BOOLEAN) {
                *approved_out = formed_response.GetBool();
              }
              GetErrorCodeMessage(std::move(formed_response), error_out,
                                  error_message_out);
              run_loop.Quit();
            }),
        base::Value());
    auto requests = GetPendingAddSuggestTokenRequests();
    if (!token) {
      ASSERT_TRUE(requests.empty());
    } else {
      ASSERT_EQ(requests.size(), 1u);
      EXPECT_EQ(requests[0]->token->contract_address, token->contract_address);
      EXPECT_TRUE(brave_wallet_tab_helper()->IsShowingBubble());
      brave_wallet_service_->NotifyAddSuggestTokenRequestsProcessed(
          approved, {token->contract_address});
    }
    run_loop.Run();
  }

  std::vector<mojom::AddSuggestTokenRequestPtr>
  GetPendingAddSuggestTokenRequests() const {
    base::RunLoop run_loop;
    std::vector<mojom::AddSuggestTokenRequestPtr> requests_out;
    brave_wallet_service_->GetPendingAddSuggestTokenRequests(
        base::BindLambdaForTesting(
            [&](std::vector<mojom::AddSuggestTokenRequestPtr> requests) {
              for (const auto& request : requests) {
                requests_out.push_back(request.Clone());
              }
              run_loop.Quit();
            }));
    run_loop.Run();
    return requests_out;
  }

  std::string GetPendingSwitchChainRequestId() {
    auto requests = json_rpc_service_->GetPendingSwitchChainRequestsSync();
    EXPECT_EQ(1u, requests.size());
    return requests[0]->request_id;
  }

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;
  std::unique_ptr<TestEventsListener> observer_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<EthereumProviderImpl> provider_;

 private:
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  content::TestWebContentsFactory factory_;
  std::unique_ptr<content::TestWebContents> web_contents_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  base::ScopedTempDir temp_dir_;
  TestingProfile profile_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<AssetRatioService> asset_ratio_service_;
  raw_ptr<TxService> tx_service_;

 protected:
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  std::unique_ptr<BraveWalletService> brave_wallet_service_;
};

TEST_F(EthereumProviderImplUnitTest, ValidateBrokenPayloads) {
  ValidateErrorCode(provider(), "", mojom::ProviderError::kInvalidParams);
  ValidateErrorCode(provider(), R"({})", mojom::ProviderError::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": []})",
                    mojom::ProviderError::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": [{}]})",
                    mojom::ProviderError::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": {}})",
                    mojom::ProviderError::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": [{
        "chainName": 'Binance1 Smart Chain',
      }]})",
                    mojom::ProviderError::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": [{
      "chainId": '0x386'
    }]})",
                    mojom::ProviderError::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": [{
      "rpcUrls": ['https://bsc-dataseed.binance.org/'],
    }]})",
                    mojom::ProviderError::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": [{
      "chainName": 'Binance1 Smart Chain',
      "rpcUrls": ['https://bsc-dataseed.binance.org/'],
    }]})",
                    mojom::ProviderError::kInvalidParams);
}

TEST_F(EthereumProviderImplUnitTest, EmptyDelegate) {
  EthereumProviderImpl provider_impl(host_content_settings_map(),
                                     brave_wallet_service_.get(), nullptr,
                                     prefs());
  ValidateErrorCode(&provider_impl,
                    R"({"params": [{
        "chainId": "0x111",
        "chainName": "Binance1 Smart Chain",
        "rpcUrls": ["https://bsc-dataseed.binance.org/"]
      }]})",
                    mojom::ProviderError::kInternalError);
}

TEST_F(EthereumProviderImplUnitTest, OnAddEthereumChain) {
  GURL url("https://brave.com");
  Navigate(url);
  base::RunLoop run_loop;

  // Test missing valid rpc URLs.
  base::RunLoop run_loop2;
  provider()->AddEthereumChain(
      R"({"params": [{
        "chainId": "0x222",
        "chainName": "Bad Chain",
        "rpcUrls": ["ftp://bar"],
      },]})",
      base::BindLambdaForTesting(
          [&run_loop2](base::Value id, base::Value formed_response,
                       const bool reject,
                       const std::string& first_allowed_account,
                       const bool update_bind_js_properties) {
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
            EXPECT_EQ(error_message,
                      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
            run_loop2.Quit();
          }),
      base::Value());
  run_loop2.Run();
}

TEST_F(EthereumProviderImplUnitTest, OnAddEthereumChainRequestCompletedError) {
  GURL url("https://brave.com");
  Navigate(url);
  base::RunLoop run_loop;
  provider()->AddEthereumChain(
      R"({"params": [{
        "chainId": "0x111",
        "chainName": "Binance1 Smart Chain",
        "rpcUrls": ["https://bsc-dataseed.binance.org/"]
      }]})",
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
            EXPECT_EQ(error_message, "test message");
            run_loop.Quit();
          }),
      base::Value());
  provider()->OnAddEthereumChainRequestCompleted("0x111", "test message");
  provider()->OnAddEthereumChainRequestCompleted("0x111", "test message");
  run_loop.Run();
}

TEST_F(EthereumProviderImplUnitTest, AddAndApproveTransaction) {
  bool callback_called = false;
  std::string tx_hash;
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);

  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(account_0->account_id);

  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      account_0->address +
      "\",\"gasPrice\":\"0x09184e72a000\","
      "\"gas\":\"0x0974\",\"to\":"
      "\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x016345785d8a0000\"}]}";
  std::optional<base::Value> response = base::JSONReader::Read(
      normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                   base::JSONParserOptions::JSON_PARSE_RFC);
  provider()->Request(
      response->Clone(),
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            if (formed_response.type() == base::Value::Type::STRING) {
              tx_hash = formed_response.GetString();
            }
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kSuccess);
            EXPECT_FALSE(tx_hash.empty());
            EXPECT_TRUE(error_message.empty());
            callback_called = true;
          }));
  browser_task_environment_.RunUntilIdle();
  const auto& chain_id =
      json_rpc_service()->GetChainIdSync(mojom::CoinType::ETH, GetOrigin());
  std::vector<mojom::TransactionInfoPtr> infos =
      GetAllTransactionInfo(account_0->account_id, chain_id);
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_EQ(infos[0]->from_account_id, account_0->account_id);
  EXPECT_EQ(*infos[0]->from_address, account_0->address);
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Unapproved);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);
  EXPECT_EQ(infos[0]->chain_id, chain_id);

  EXPECT_EQ(*GetTransactionInfo(infos[0]->id), *infos[0]);
  EXPECT_TRUE(GetTransactionInfo("unknown_id").is_null());

  // Set an interceptor and just fake a common response for
  // eth_getTransactionCount and eth_sendRawTransaction
  SetInterceptor("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0\"}");

  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;

  EXPECT_TRUE(
      ApproveTransaction(chain_id, infos[0]->id, &error, &error_message));
  browser_task_environment_.RunUntilIdle();

  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  EXPECT_TRUE(callback_called);
  infos = GetAllTransactionInfo(account_0->account_id, chain_id);
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_EQ(infos[0]->from_account_id, account_0->account_id);
  EXPECT_EQ(*infos[0]->from_address, account_0->address);
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Submitted);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);
}

TEST_F(EthereumProviderImplUnitTest, AddAndApproveTransactionError) {
  // We don't need to check every error type since that is checked by
  // eth_tx_manager_unittest but make sure an error type is handled
  // correctly.
  bool callback_called = false;
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);

  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(account_0->account_id);
  // Bad address
  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      account_0->address +
      "\",\"gasPrice\":\"0x09184e72a000\","
      "\"gas\":\"0x0974\",\"to\":\"0xbe8\","
      "\"value\":\"0x016345785d8a0000\"}]}";
  std::optional<base::Value> response = base::JSONReader::Read(
      normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                   base::JSONParserOptions::JSON_PARSE_RFC);
  provider()->Request(
      response->Clone(),
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            std::string hash;
            if (formed_response.type() == base::Value::Type::STRING) {
              hash = formed_response.GetString();
            }
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_NE(error, mojom::ProviderError::kSuccess);
            EXPECT_TRUE(hash.empty());
            EXPECT_FALSE(error_message.empty());
            callback_called = true;
          }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthereumProviderImplUnitTest, AddAndApproveTransactionNoPermission) {
  bool callback_called = false;
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);

  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      account_0->address +
      "\",\"gasPrice\":\"0x09184e72a000\","
      "\"gas\":\"0x0974\",\"to\":"
      "\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x016345785d8a0000\"}]}";
  std::optional<base::Value> response = base::JSONReader::Read(
      normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                   base::JSONParserOptions::JSON_PARSE_RFC);
  provider()->Request(
      response->Clone(),
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            std::string hash;
            if (formed_response.type() == base::Value::Type::STRING) {
              hash = formed_response.GetString();
            }
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_NE(error, mojom::ProviderError::kSuccess);
            EXPECT_TRUE(hash.empty());
            EXPECT_FALSE(error_message.empty());
            callback_called = true;
          }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthereumProviderImplUnitTest, AddAndApprove1559Transaction) {
  bool callback_called = false;
  std::string tx_hash;
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);

  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(account_0->account_id);
  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      account_0->address +
      "\",\"maxFeePerGas\":\"0x1\",\"maxPriorityFeePerGas\":\"0x1\","
      "\"gas\":\"0x1\",\"to\":\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x00\"}]}";
  std::optional<base::Value> response = base::JSONReader::Read(
      normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                   base::JSONParserOptions::JSON_PARSE_RFC);
  provider()->Request(
      response->Clone(),
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            if (formed_response.type() == base::Value::Type::STRING) {
              tx_hash = formed_response.GetString();
            }
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kSuccess);
            EXPECT_FALSE(tx_hash.empty());
            EXPECT_TRUE(error_message.empty());
            callback_called = true;
          }));
  browser_task_environment_.RunUntilIdle();
  const auto& chain_id =
      json_rpc_service()->GetChainIdSync(mojom::CoinType::ETH, GetOrigin());
  std::vector<mojom::TransactionInfoPtr> infos =
      GetAllTransactionInfo(account_0->account_id, chain_id);
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_EQ(infos[0]->from_account_id, account_0->account_id);
  EXPECT_EQ(*infos[0]->from_address, account_0->address);
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Unapproved);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);
  EXPECT_EQ(infos[0]->chain_id, chain_id);

  // Set an interceptor and just fake a common response for
  // eth_getTransactionCount and eth_sendRawTransaction
  SetInterceptor("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0\"}");

  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;

  EXPECT_TRUE(
      ApproveTransaction(chain_id, infos[0]->id, &error, &error_message));
  browser_task_environment_.RunUntilIdle();

  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  EXPECT_TRUE(callback_called);
  infos = GetAllTransactionInfo(account_0->account_id, chain_id);
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_EQ(infos[0]->from_account_id, account_0->account_id);
  EXPECT_EQ(*infos[0]->from_address, account_0->address);
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Submitted);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);
  EXPECT_EQ(infos[0]->chain_id, chain_id);
}

TEST_F(EthereumProviderImplUnitTest, AddAndApprove1559TransactionNoChainId) {
  std::string tx_hash;
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);

  GURL url("https://brave.com");
  Navigate(url);
  SetNetwork(mojom::kSepoliaChainId, GetOrigin());
  // Wait for EthTxStateManager::ChainChangedEvent to be called.
  browser_task_environment_.RunUntilIdle();

  AddEthereumPermission(account_0->account_id);
  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      account_0->address +
      "\",\"maxFeePerGas\":\"0x1\",\"maxPriorityFeePerGas\":\"0x1\","
      "\"gas\":\"0x1\",\"to\":\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x00\"}]}";
  std::optional<base::Value> response = base::JSONReader::Read(
      normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                   base::JSONParserOptions::JSON_PARSE_RFC);
  provider()->Request(
      response->Clone(),
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            tx_hash.clear();
            if (formed_response.type() == base::Value::Type::STRING) {
              tx_hash = formed_response.GetString();
            }
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kSuccess);
            EXPECT_FALSE(tx_hash.empty());
            EXPECT_TRUE(error_message.empty());
          }));
  browser_task_environment_.RunUntilIdle();
  provider()->Request(
      response->Clone(),
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            tx_hash.clear();
            if (formed_response.type() == base::Value::Type::STRING) {
              tx_hash = formed_response.GetString();
            }
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kSuccess);
            EXPECT_FALSE(tx_hash.empty());
            EXPECT_TRUE(error_message.empty());
          }));
  browser_task_environment_.RunUntilIdle();
  std::vector<mojom::TransactionInfoPtr> infos =
      GetAllTransactionInfo(account_0->account_id, mojom::kSepoliaChainId);
  ASSERT_EQ(infos.size(), 2UL);
  ASSERT_TRUE(infos[0]->tx_data_union->is_eth_tx_data_1559());
  EXPECT_EQ(infos[0]->tx_data_union->get_eth_tx_data_1559()->chain_id,
            mojom::kSepoliaChainId);
  EXPECT_EQ(infos[1]->tx_data_union->get_eth_tx_data_1559()->chain_id,
            mojom::kSepoliaChainId);
}

TEST_F(EthereumProviderImplUnitTest, AddAndApprove1559TransactionError) {
  // We don't need to check every error type since that is checked by
  // eth_tx_manager_unittest but make sure an error type is handled
  // correctly.
  bool callback_called = false;
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);

  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(account_0->account_id);
  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      account_0->address +
      "\",\"maxFeePerGas\":\"0x0\",\"maxPriorityFeePerGas\":\"0x0\","
      "\"gasPrice\":\"0x01\", "
      "\"gas\":\"0x00\",\"to\":\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x00\"}]}";
  std::optional<base::Value> response = base::JSONReader::Read(
      normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                   base::JSONParserOptions::JSON_PARSE_RFC);
  provider()->Request(
      response->Clone(),
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            std::string tx_hash;
            if (formed_response.type() == base::Value::Type::STRING) {
              tx_hash = formed_response.GetString();
            }
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_NE(error, mojom::ProviderError::kSuccess);
            EXPECT_TRUE(tx_hash.empty());
            EXPECT_FALSE(error_message.empty());
            callback_called = true;
          }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthereumProviderImplUnitTest, AddAndApprove1559TransactionNoPermission) {
  bool callback_called = false;
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);

  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      account_0->address +
      "\",\"maxFeePerGas\":\"0x0\",\"maxPriorityFeePerGas\":\"0x0\","
      "\"gas\":\"0x00\",\"to\":\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x00\"}]}";
  std::optional<base::Value> response = base::JSONReader::Read(
      normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                   base::JSONParserOptions::JSON_PARSE_RFC);
  provider()->Request(
      response->Clone(),
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            std::string tx_hash;
            if (formed_response.type() == base::Value::Type::STRING) {
              tx_hash = formed_response.GetString();
            }
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_NE(error, mojom::ProviderError::kSuccess);
            EXPECT_TRUE(tx_hash.empty());
            EXPECT_FALSE(error_message.empty());
            callback_called = true;
          }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthereumProviderImplUnitTest, RequestEthereumPermissionNotNewSetup) {
  bool new_setup_callback_called = false;
  SetCallbackForNewSetupNeededForTesting(
      base::BindLambdaForTesting([&]() { new_setup_callback_called = true; }));
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  auto address_0 = base::ToLowerASCII(account_0->address);

  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(account_0->account_id);
  base::RunLoop run_loop;
  EXPECT_THAT(RequestEthereumPermissions(), ElementsAre(address_0));
  // Make sure even with a delay the new setup callback is not called.
  browser_task_environment_.RunUntilIdle();
  EXPECT_FALSE(new_setup_callback_called);
}

TEST_F(EthereumProviderImplUnitTest, RequestEthereumPermissionsNoPermission) {
  bool new_setup_callback_called = false;
  SetCallbackForNewSetupNeededForTesting(
      base::BindLambdaForTesting([&]() { new_setup_callback_called = true; }));
  bool permission_callback_called = false;
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);

  provider()->RequestEthereumPermissions(
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_NE(error, mojom::ProviderError::kSuccess);
            EXPECT_FALSE(error_message.empty());
            permission_callback_called = true;
          }),
      base::Value(), "", GetOrigin());
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(permission_callback_called);
  EXPECT_FALSE(new_setup_callback_called);
}

TEST_F(EthereumProviderImplUnitTest, RequestEthereumPermissionsNoWallet) {
  bool new_setup_callback_called = false;
  SetCallbackForNewSetupNeededForTesting(
      base::BindLambdaForTesting([&]() { new_setup_callback_called = true; }));
  base::RunLoop run_loop;
  provider()->RequestEthereumPermissions(
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_NE(error, mojom::ProviderError::kSuccess);
            EXPECT_FALSE(error_message.empty());
            run_loop.Quit();
          }),
      base::Value(), "", GetOrigin());
  run_loop.Run();
  EXPECT_TRUE(new_setup_callback_called);

  // Setup is called at most once
  new_setup_callback_called = false;
  SetCallbackForNewSetupNeededForTesting(
      base::BindLambdaForTesting([&]() { new_setup_callback_called = true; }));
  base::RunLoop run_loop2;
  provider()->RequestEthereumPermissions(
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            mojom::ProviderError error = mojom::ProviderError::kUnknown;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_NE(error, mojom::ProviderError::kSuccess);
            EXPECT_FALSE(error_message.empty());
            run_loop2.Quit();
          }),
      base::Value(), "", GetOrigin());
  run_loop2.Run();
  EXPECT_FALSE(new_setup_callback_called);
}

TEST_F(EthereumProviderImplUnitTest, RequestEthereumPermissionsWithAccounts) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  auto account_1 = GetAccountUtils().EnsureEthAccount(1);
  auto account_2 = GetAccountUtils().EnsureEthAccount(2);

  auto address_0 = base::ToLowerASCII(account_0->address);
  auto address_1 = base::ToLowerASCII(account_1->address);

  GURL url("https://brave.com");
  Navigate(url);

  // Allowing 1 account should return that account for allowed accounts
  AddEthereumPermission(account_0->account_id);
  EXPECT_THAT(RequestEthereumPermissions(), ElementsAre(address_0));

  // Multiple accounts can be returned
  AddEthereumPermission(account_1->account_id);
  EXPECT_THAT(RequestEthereumPermissions(), ElementsAre(address_0, address_1));

  // Resetting permissions should return the remaining allowed account
  ResetEthereumPermission(account_1->account_id);
  EXPECT_THAT(RequestEthereumPermissions(), ElementsAre(address_0));

  // Selected account should filter the accounts returned
  AddEthereumPermission(account_1->account_id);
  SetSelectedAccount(account_0->account_id);
  EXPECT_THAT(RequestEthereumPermissions(), ElementsAre(address_0));
  SetSelectedAccount(account_1->account_id);
  EXPECT_THAT(RequestEthereumPermissions(), ElementsAre(address_1));
  SetSelectedAccount(account_2->account_id);
  EXPECT_THAT(RequestEthereumPermissions(), ElementsAre(address_0, address_1));

  // CONTENT_SETTING_BLOCK will rule out previous granted permission.
  host_content_settings_map()->SetContentSettingDefaultScope(
      url, url, ContentSettingsType::BRAVE_ETHEREUM, CONTENT_SETTING_BLOCK);
  base::RunLoop run_loop;
  provider()->RequestEthereumPermissions(
      base::BindLambdaForTesting([&](base::Value id,
                                     base::Value formed_response,
                                     const bool reject,
                                     const std::string& first_allowed_account,
                                     const bool update_bind_js_properties) {
        mojom::ProviderError error = mojom::ProviderError::kUnknown;
        std::string error_message;
        GetErrorCodeMessage(std::move(formed_response), &error, &error_message);
        EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
        EXPECT_EQ(error_message,
                  l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
        run_loop.Quit();
      }),
      base::Value(), "", GetOrigin());
  run_loop.Run();
  // When CONTENT_SETTING_BLOCK is removed, previously granted permission works
  // again.
  host_content_settings_map()->SetContentSettingDefaultScope(
      url, url, ContentSettingsType::BRAVE_ETHEREUM, CONTENT_SETTING_DEFAULT);
  EXPECT_THAT(RequestEthereumPermissions(), ElementsAre(address_0, address_1));
}

TEST_F(EthereumProviderImplUnitTest, RequestEthereumPermissionsLocked) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  auto address_0 = base::ToLowerASCII(account_0->address);

  GURL url("https://brave.com");
  Navigate(url);

  // Allowing 1 account should return that account for allowed accounts
  AddEthereumPermission(account_0->account_id);
  Lock();
  // Allowed accounts are empty when locked
  EXPECT_THAT(GetAllowedAccounts(false), IsEmpty());
  EXPECT_THAT(GetAllowedAccounts(true), ElementsAre(address_0));
  std::vector<std::string> allowed_accounts;
  base::RunLoop run_loop;
  provider()->RequestEthereumPermissions(
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            if (formed_response.GetList().size() != 0) {
              std::string stylesheet = "";
              for (auto& account : formed_response.GetList()) {
                allowed_accounts.push_back(account.GetString());
              }
            }
            run_loop.Quit();
          }),
      base::Value(), "", GetOrigin());
  // Wait for KeyringService::GetSelectedAccount called by
  // BraveWalletProviderDelegateImpl::GetAllowedAccounts
  browser_task_environment_.RunUntilIdle();

  EXPECT_TRUE(keyring_service()->HasPendingUnlockRequest());
  // Allowed accounts are still empty when locked
  EXPECT_THAT(GetAllowedAccounts(false), IsEmpty());
  EXPECT_THAT(GetAllowedAccounts(true), ElementsAre(address_0));
  Unlock();
  run_loop.Run();

  EXPECT_FALSE(keyring_service()->HasPendingUnlockRequest());
  EXPECT_THAT(allowed_accounts, ElementsAre(address_0));
}

TEST_F(EthereumProviderImplUnitTest, SignMessage) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  Navigate(GURL("https://brave.com"));
  for (const auto& message : {
           std::string("0x1234"),
           ToHex(GetSIWEMessage("https://brave.com", account_0->address,
                                "https://brave.com/login", "1")),
       }) {
    SCOPED_TRACE(message);
    std::string signature;
    mojom::ProviderError error = mojom::ProviderError::kUnknown;
    std::string error_message;
    SignMessage(std::nullopt, "1234", message, &signature, &error,
                &error_message);
    EXPECT_TRUE(signature.empty());
    EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
    EXPECT_EQ(error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

    SignMessage(std::nullopt, "0x12345678", message, &signature, &error,
                &error_message);
    EXPECT_TRUE(signature.empty());
    EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
    EXPECT_EQ(error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

    SignMessage(std::nullopt, GetAccountUtils().EthUnkownAccount()->address,
                message, &signature, &error, &error_message);
    EXPECT_TRUE(signature.empty());
    EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
    EXPECT_EQ(error_message, l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));

    // No permission
    SignMessage(std::nullopt, account_0->address, message, &signature, &error,
                &error_message);
    EXPECT_TRUE(signature.empty());
    EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
    EXPECT_EQ(error_message, l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
    AddEthereumPermission(account_0->account_id);
    SignMessage(true, account_0->address, message, &signature, &error,
                &error_message);

    EXPECT_FALSE(signature.empty());
    EXPECT_EQ(error, mojom::ProviderError::kSuccess);
    EXPECT_TRUE(error_message.empty());

    // User reject request
    SignMessage(false, account_0->address, message, &signature, &error,
                &error_message);
    EXPECT_TRUE(signature.empty());
    EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
    EXPECT_EQ(error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

    Lock();

    // nullopt for the first param here because we don't AddSignMessageRequest
    // whent here are no accounts returned.
    SignMessage(std::nullopt, account_0->address, message, &signature, &error,
                &error_message);
    EXPECT_TRUE(signature.empty());
    EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
    EXPECT_EQ(error_message, l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));

    Unlock();
    ResetEthereumPermission(account_0->account_id);
  }
}

TEST_F(EthereumProviderImplUnitTest, SignMessageWithTypedDataStructure) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  for (const auto& method : {"personal_sign", "eth_sign"}) {
    std::string request_payload_json = base::StringPrintf(
        R"({"id":1, "jsonrpc:": "2.0", "method": "%s",
          "params": ["%s", "{
            \"types\": {
              \"EIP712Domain\": [
                { \"name\": \"name\", \"type\": \"string\" },
                { \"name\": \"version\", \"type\": \"string\" },
                { \"name\": \"chainId\", \"type\": \"uint256\" },
                { \"name\": \"verifyingContract\", \"type\": \"address\" }
              ],
              \"Person\": [
                { \"name\": \"name\", \"type\": \"string\" },
                { \"name\": \"wallet\", \"type\": \"address\" }
              ],
              \"Mail\": [
                { \"name\": \"from\", \"type\": \"Person\" },
                { \"name\": \"to\", \"type\": \"Person\" },
                { \"name\": \"contents\", \"type\": \"string\" }
              ]
            },
            \"primaryType\": \"Mail\",
            \"domain\": {
              \"name\": \"Ether Mail\",
              \"version\": \"1\",
              \"chainId\": 1,
              \"verifyingContract\":
                \"0xCcCCccccCCCCcCCCCCCcCcCccCcCCCcCcccccccC\"
            },
            \"message\": {
              \"from\": { \"name\": \"Cow\",
                \"wallet\": \"0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826\" },
              \"to\": { \"name\": \"Bob\",
                \"wallet\": \"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB\" },
              \"contents\": \"Hello, Bob!\"
            }
          }"]})",
        method, account_0->address.c_str());
    std::optional<base::Value> request_payload = base::JSONReader::Read(
        request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                  base::JSONParserOptions::JSON_PARSE_RFC);
    auto response = CommonRequestOrSendAsync(request_payload.value());

    mojom::ProviderError error_code;
    std::string error_message;
    GetErrorCodeMessage(std::move(response.second), &error_code,
                        &error_message);
    EXPECT_EQ(response.first, true);
    EXPECT_EQ(error_code, mojom::ProviderError::kInvalidParams);
    EXPECT_EQ(error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  }
}

TEST_F(EthereumProviderImplUnitTest, SigninWithEthereumError) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  auto account_1 = GetAccountUtils().EnsureEthAccount(1);
  std::string signature;
  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;
  CreateBraveWalletTabHelper();
  brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);
  Navigate(GURL("https://brave.com"));
  AddEthereumPermission(account_0->account_id);

  struct {
    std::string domain;
    std::string address;
    std::string uri;
    std::string network;
    mojom::SignMessageErrorType siwe_err_type;
    std::string siwe_err_msg;
    std::optional<std::string> chain_id;
    mojom::ProviderError provider_err;
    std::string provider_err_msg;
  } cases[]{
      {"https://brave.com", account_0->address, "https://brave.com/login",
       "5566", mojom::SignMessageErrorType::kChainIdMismatched,
       l10n_util::GetStringFUTF8(
           IDS_BRAVE_WALLET_SIGN_MESSAGE_MISMATCH_ERR,
           l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_NETWORK),
           base::StrCat({l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_CHAIN_ID),
                         u": 5566"})),
       std::string("5566"), mojom::ProviderError::kInternalError,
       l10n_util::GetStringFUTF8(
           IDS_BRAVE_WALLET_SIGN_MESSAGE_CHAIN_ID_MISMATCH, u"5566")},
      {"https://brave.com", account_1->address, "https://brave.com/login", "1",
       mojom::SignMessageErrorType::kAccountMismatched,
       l10n_util::GetStringFUTF8(
           IDS_BRAVE_WALLET_SIGN_MESSAGE_MISMATCH_ERR,
           l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_ACCOUNT),
           base::ASCIIToUTF16(account_1->address)),
       std::nullopt, mojom::ProviderError::kInternalError,
       l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_ACCOUNT_MISMATCH,
                                 base::ASCIIToUTF16(account_1->address))},
      {"https://example.com", account_0->address, "https://brave.com/login",
       "1", mojom::SignMessageErrorType::kDomainMismatched,
       l10n_util::GetStringFUTF8(
           IDS_BRAVE_WALLET_SIGN_MESSAGE_MISMATCH_ERR,
           l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_DOMAIN),
           u"https://example.com"),
       std::nullopt, mojom::ProviderError::kInternalError,
       l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_DOMAIN_MISMATCH,
                                 u"https://example.com")},
  };
  for (const auto& invalid_case : cases) {
    const std::string& siwe_message =
        GetSIWEMessage(invalid_case.domain, invalid_case.address,
                       invalid_case.uri, invalid_case.network);
    SCOPED_TRACE(siwe_message);
    SignMessage(true, account_0->address, ToHex(siwe_message), &signature,
                &error, &error_message);
    EXPECT_TRUE(brave_wallet_tab_helper()->IsShowingBubble());
    EXPECT_EQ(GetSignMessageErrorQueueFront()->origin_info,
              MakeOriginInfo(GetOrigin()));
    EXPECT_EQ(GetSignMessageErrorQueueFront()->type,
              invalid_case.siwe_err_type);
    EXPECT_EQ(GetSignMessageErrorQueueFront()->localized_err_msg,
              invalid_case.siwe_err_msg);
    EXPECT_EQ(GetSignMessageErrorQueueFront()->chain_id, invalid_case.chain_id);

    EXPECT_TRUE(signature.empty());
    EXPECT_EQ(error, invalid_case.provider_err);
    EXPECT_EQ(error_message, invalid_case.provider_err_msg);

    brave_wallet_service_->NotifySignMessageErrorProcessed(
        GetSignMessageErrorQueueFront()->id);
  }
}

TEST_F(EthereumProviderImplUnitTest, RecoverAddress) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);

  std::string signature;
  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;
  const std::vector<std::string> addresses = GetAddresses();

  std::string message = "0x68656c6c6f20776f726c64";
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(account_0->account_id);
  SignMessage(true, addresses[0], message, &signature, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  // 132 = 65 * 2 chars per byte + 2 chars for 0x
  EXPECT_EQ(signature.size(), 132UL);

  // Keyring can be locked
  Lock();

  std::string out_address;
  RecoverAddress(message, signature, &out_address, &error, &error_message);
  EXPECT_EQ(out_address, addresses[0]);
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());

  // Must have hex input at this point
  // text input is converted in ParsePersonalEcRecoverParams
  RecoverAddress("hello world", signature, &out_address, &error,
                 &error_message);
  EXPECT_EQ(out_address, "");
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Invalid signature
  RecoverAddress(message, "zzzzz", &out_address, &error, &error_message);
  EXPECT_EQ(out_address, "");
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Signature too long
  signature += "12";
  RecoverAddress("hello world", signature, &out_address, &error,
                 &error_message);
  EXPECT_EQ(out_address, "");
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
}

TEST_F(EthereumProviderImplUnitTest, SignTypedMessage) {
  EXPECT_EQ(
      json_rpc_service()->GetChainIdSync(mojom::CoinType::ETH, std::nullopt),
      "0x1");
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  std::string signature;
  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;
  base::Value::Dict domain;
  std::vector<uint8_t> domain_hash = DecodeHexHash(
      "f2cee375fa42b42143804025fc449deafd50cc031ca257e0b194a650a912090f");
  std::vector<uint8_t> primary_hash = DecodeHexHash(
      "c52c0ee5d84264471806290a3f2c4cecfc5490626bf912d01f240d7a274b371e");
  domain.Set("chainId", 1);
  SignTypedMessage(std::nullopt, "1234", "{...}", domain_hash, primary_hash,
                   domain.Clone(), nullptr, &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  SignTypedMessage(std::nullopt, "0x12345678", "{...}", domain_hash,
                   primary_hash, domain.Clone(), nullptr, &signature, &error,
                   &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  const std::string address_0 = account_0->address;

  // not valid domain hash
  SignTypedMessage(std::nullopt, address_0, "{...}", {}, primary_hash,
                   domain.Clone(), nullptr, &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // not valid primary hash
  SignTypedMessage(std::nullopt, address_0, "{...}", domain_hash, {},
                   domain.Clone(), nullptr, &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  domain.Set("chainId", 11155111);
  std::string chain_id = "0xaa36a7";
  // not active network
  SignTypedMessage(std::nullopt, address_0, "{...}", domain_hash, primary_hash,
                   domain.Clone(), nullptr, &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInternalError);
  EXPECT_EQ(error_message, l10n_util::GetStringFUTF8(
                               IDS_BRAVE_WALLET_SIGN_MESSAGE_CHAIN_ID_MISMATCH,
                               base::ASCIIToUTF16(chain_id)));
  domain.Set("chainId", 1);

  SignTypedMessage(std::nullopt, address_0, "{...}", domain_hash, primary_hash,
                   domain.Clone(), nullptr, &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
  EXPECT_EQ(error_message, l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));

  // No permission
  ASSERT_FALSE(address_0.empty());
  SignTypedMessage(std::nullopt, address_0, "{...}", domain_hash, primary_hash,
                   domain.Clone(), nullptr, &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
  EXPECT_EQ(error_message, l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(account_0->account_id);
  SignTypedMessage(true, address_0, "{...}", domain_hash, primary_hash,
                   domain.Clone(), nullptr, &signature, &error, &error_message);

  EXPECT_FALSE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());

  // User reject request
  SignTypedMessage(false, address_0, "{...}", domain_hash, primary_hash,
                   domain.Clone(), nullptr, &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  // not valid eip712 domain hash
  SignTypedMessage(std::nullopt, address_0, "{...}", DecodeHexHash("brave"),
                   primary_hash, domain.Clone(), nullptr, &signature, &error,
                   &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  // not valid eip712 primary hash
  SignTypedMessage(std::nullopt, address_0, "{...}", domain_hash,
                   DecodeHexHash("primary"), domain.Clone(), nullptr,
                   &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  keyring_service()->Lock();

  // nullopt for the first param here because we don't AddSignMessageRequest
  // when here are no accounts returned.
  SignTypedMessage(std::nullopt, address_0, "{...}", domain_hash, primary_hash,
                   domain.Clone(), nullptr, &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
  EXPECT_EQ(error_message, l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
}

TEST_F(EthereumProviderImplUnitTest, SignMessageRequestQueue) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  std::string hardware = "0xA99D71De40D67394eBe68e4D0265cA6C9D421029";
  auto account_hw = AddHardwareAccount(hardware);

  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(account_0->account_id);
  AddEthereumPermission(account_hw->account_id);

  // Select account that is not participating in signing process.
  // If there is allowed account which is also selected then only
  // this account may be used for signin process.
  // another_hd_account is not allowed because it has no permission.
  // Also see EthereumProviderImpl.FilterAccounts method.
  auto account_1 = GetAccountUtils().EnsureEthAccount(1);
  SetSelectedAccount(account_1->account_id);

  const std::string message1 = "0x68656c6c6f20776f726c64";
  const std::string message2 = "0x4120756e69636f646520c68e20737472696e6720c3b1";
  const std::string message3 = "0xbeef03";
  int id1 = SignMessageRequest(account_0->address, message1);
  int id2 = SignMessageRequest(account_0->address, message2);
  int id3 = SignMessageRequest(account_hw->address, message3);

  std::vector<uint8_t> message_bytes1;
  std::vector<uint8_t> message_bytes2;
  std::vector<uint8_t> message_bytes3;
  ASSERT_TRUE(PrefixedHexStringToBytes(message1, &message_bytes1));
  ASSERT_TRUE(PrefixedHexStringToBytes(message2, &message_bytes2));
  ASSERT_TRUE(PrefixedHexStringToBytes(message3, &message_bytes3));
  const std::string message1_in_queue = "hello world";
  const std::string message2_in_queue = "A unicode Ǝ string ñ";
  const std::string message3_in_queue = "0xbeef03";

  EXPECT_EQ(GetSignMessageQueueSize(), 3u);
  EXPECT_EQ(GetSignMessageQueueFront()->id, id1);
  EXPECT_EQ(GetSignMessageQueueFront()
                ->sign_data->get_eth_standard_sign_data()
                ->message,
            message1_in_queue);
  {
    auto queue = GetPendingSignMessageRequests();
    ASSERT_EQ(queue.size(), 3u);
    EXPECT_EQ(queue[0]->id, id1);
    EXPECT_EQ(queue[0]->sign_data->get_eth_standard_sign_data()->message,
              message1_in_queue);
    EXPECT_EQ(queue[1]->id, id2);
    EXPECT_EQ(queue[1]->sign_data->get_eth_standard_sign_data()->message,
              message2_in_queue);
    EXPECT_EQ(queue[2]->id, id3);
    EXPECT_EQ(queue[2]->sign_data->get_eth_standard_sign_data()->message,
              message3_in_queue);
  }

  // wrong order
  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id2, nullptr,
                                                           std::nullopt);
  EXPECT_EQ(GetSignMessageQueueSize(), 3u);
  EXPECT_EQ(GetSignMessageQueueFront()->id, id1);
  EXPECT_EQ(GetSignMessageQueueFront()
                ->sign_data->get_eth_standard_sign_data()
                ->message,
            message1_in_queue);

  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id3, nullptr,
                                                           std::nullopt);
  EXPECT_EQ(GetSignMessageQueueSize(), 3u);
  EXPECT_EQ(GetSignMessageQueueFront()->id, id1);
  EXPECT_EQ(GetSignMessageQueueFront()
                ->sign_data->get_eth_standard_sign_data()
                ->message,
            message1_in_queue);

  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id1, nullptr,
                                                           std::nullopt);
  EXPECT_EQ(GetSignMessageQueueSize(), 2u);
  EXPECT_EQ(GetSignMessageQueueFront()->id, id2);
  EXPECT_EQ(GetSignMessageQueueFront()
                ->sign_data->get_eth_standard_sign_data()
                ->message,
            message2_in_queue);
  {
    auto queue = GetPendingSignMessageRequests();
    ASSERT_EQ(queue.size(), 2u);
    EXPECT_EQ(queue[0]->id, id2);
    EXPECT_EQ(queue[0]->sign_data->get_eth_standard_sign_data()->message,
              message2_in_queue);
    EXPECT_EQ(queue[1]->id, id3);
    EXPECT_EQ(queue[1]->sign_data->get_eth_standard_sign_data()->message,
              message3_in_queue);
  }

  // old id
  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id1, nullptr,
                                                           std::nullopt);
  EXPECT_EQ(GetSignMessageQueueSize(), 2u);
  EXPECT_EQ(GetSignMessageQueueFront()->id, id2);
  EXPECT_EQ(GetSignMessageQueueFront()
                ->sign_data->get_eth_standard_sign_data()
                ->message,
            message2_in_queue);

  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id2, nullptr,
                                                           std::nullopt);
  EXPECT_EQ(GetSignMessageQueueSize(), 1u);
  EXPECT_EQ(GetSignMessageQueueFront()->id, id3);
  EXPECT_EQ(GetSignMessageQueueFront()
                ->sign_data->get_eth_standard_sign_data()
                ->message,
            message3_in_queue);
  {
    auto queue = GetPendingSignMessageRequests();
    ASSERT_EQ(queue.size(), 1u);
    EXPECT_EQ(queue[0]->id, id3);
    EXPECT_EQ(queue[0]->sign_data->get_eth_standard_sign_data()->message,
              message3_in_queue);
  }

  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id3, nullptr,
                                                           std::nullopt);
  EXPECT_EQ(GetSignMessageQueueSize(), 0u);
  EXPECT_EQ(GetPendingSignMessageRequests().size(), 0u);
}

TEST_F(EthereumProviderImplUnitTest, ChainChangedEvent) {
  GURL url("https://brave.com");
  Navigate(url);

  EXPECT_CALL(*observer_, ChainChangedEvent(mojom::kSepoliaChainId)).Times(1);
  SetNetwork(mojom::kSepoliaChainId, std::nullopt);
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(observer_.get()));

  // Works a second time
  EXPECT_CALL(*observer_, ChainChangedEvent(mojom::kMainnetChainId)).Times(1);
  SetNetwork(mojom::kMainnetChainId, std::nullopt);
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(observer_.get()));

  EXPECT_CALL(*observer_, ChainChangedEvent(mojom::kSepoliaChainId)).Times(1);
  SetNetwork(mojom::kSepoliaChainId, GetOrigin());
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(observer_.get()));

  // SetNetwork for other origin will be ignored.
  EXPECT_CALL(*observer_, ChainChangedEvent(testing::_)).Times(0);
  SetNetwork(mojom::kLocalhostChainId,
             url::Origin::Create(GURL("https://a.com")));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(observer_.get()));
}

TEST_F(EthereumProviderImplUnitTest, AccountsChangedEvent) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  auto account_1 = GetAccountUtils().EnsureEthAccount(1);
  auto address_0 = base::ToLowerASCII(account_0->address);

  GURL url("https://brave.com");
  Navigate(url);
  EXPECT_FALSE(observer_->AccountsChangedFired());
  AddEthereumPermission(account_0->account_id);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_THAT(observer_->GetLowercaseAccounts(), ElementsAre(address_0));
  observer_->Reset();

  // Locking the account fires an event change with no accounts
  Lock();
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ(std::vector<std::string>(), observer_->GetLowercaseAccounts());
  observer_->Reset();

  // Unlocking also fires an event wit the same account list as before
  Unlock();
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_THAT(observer_->GetLowercaseAccounts(), ElementsAre(address_0));
  observer_->Reset();

  // Does not fire for a different origin that has no permissions
  Navigate(GURL("https://bravesoftware.com"));
  AddEthereumPermission(account_1->account_id);
  SetSelectedAccount(account_0->account_id);
  EXPECT_FALSE(observer_->AccountsChangedFired());
}

TEST_F(EthereumProviderImplUnitTest, EthSubscribe) {
  CreateWallet();

  // Unsupported subscription type
  std::string request_payload_json =
      R"({"id":1,"jsonrpc:": "2.0","method":"eth_subscribe",
          "params": ["foo"]})";
  std::optional<base::Value> request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  auto response = CommonRequestOrSendAsync(request_payload.value());

  mojom::ProviderError error_code;
  std::string error_message;
  GetErrorCodeMessage(std::move(response.second), &error_code, &error_message);
  EXPECT_EQ(response.first, true);
  EXPECT_EQ(error_code, mojom::ProviderError::kInternalError);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_UNSUPPORTED_SUBSCRIPTION_TYPE));

  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();

        auto header_value = request.headers.GetHeader("X-Eth-Method");
        ASSERT_TRUE(header_value);
        std::string content;
        if (*header_value == "eth_blockNumber" ||
            *header_value == "getBlockHeight") {
          content = R"({"id":1,"jsonrpc":"2.0","result":"0x131131"})";
        } else if (*header_value == "eth_getBlockByNumber") {
          content = R"({"id":1,"jsonrpc":"2.0","result":{"difficulty":"0x1"}})";
        }
        url_loader_factory_.AddResponse(request.url.spec(), content);
      }));

  // Subscribing to newHeads
  request_payload_json =
      R"({"id":1,"jsonrpc:": "2.0","method":"eth_subscribe",
          "params": ["newHeads"]})";
  request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_EQ(response.first, false);
  EXPECT_TRUE(response.second.is_string());
  std::string first_subscription = *response.second.GetIfString();
  browser_task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));
  EXPECT_TRUE(observer_->MessageEventFired());
  base::Value rv = observer_->GetLastMessage();
  ASSERT_TRUE(rv.is_dict());
  auto& dict = rv.GetDict();
  std::string* difficulty = dict.FindString("difficulty");
  ASSERT_TRUE(difficulty);
  EXPECT_EQ(*difficulty, "0x1");

  // Make a second subscription
  request_payload_json =
      R"({"id":1,"jsonrpc:": "2.0","method":"eth_subscribe",
          "params": ["newHeads"]})";
  request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_EQ(response.first, false);
  EXPECT_TRUE(response.second.is_string());
  std::string second_subscription = *response.second.GetIfString();

  // The first unsubscribe should not stop the block tracker
  request_payload_json = base::StringPrintf(R"({"id":1,"jsonrpc:": "2.0",
                              "method":"eth_unsubscribe",
                              "params": ["%s"]})",
                                            first_subscription.c_str());
  request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  response = CommonRequestOrSendAsync(request_payload.value());

  const auto& chain_id =
      json_rpc_service()->GetChainIdSync(mojom::CoinType::ETH, GetOrigin());
  EXPECT_TRUE(provider_->eth_block_tracker_.IsRunning(chain_id));

  // The second unsubscribe should stop the block tracker
  request_payload_json = base::StringPrintf(R"({"id":1,"jsonrpc:": "2.0",
                              "method":"eth_unsubscribe",
                              "params": ["%s"]})",
                                            second_subscription.c_str());
  request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_FALSE(provider_->eth_block_tracker_.IsRunning(chain_id));
}

TEST_F(EthereumProviderImplUnitTest, EthSubscribeLogs) {
  CreateWallet();

  // Unsupported subscription type
  std::string request_payload_json =
      R"({"id":1,"jsonrpc:": "2.0","method":"eth_subscribe",
          "params": ["foo"]})";
  std::optional<base::Value> request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  auto response = CommonRequestOrSendAsync(request_payload.value());

  mojom::ProviderError error_code;
  std::string error_message;
  GetErrorCodeMessage(std::move(response.second), &error_code, &error_message);
  EXPECT_EQ(response.first, true);
  EXPECT_EQ(error_code, mojom::ProviderError::kInternalError);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_UNSUPPORTED_SUBSCRIPTION_TYPE));

  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();

        auto header_value = request.headers.GetHeader("X-Eth-Method");
        ASSERT_TRUE(header_value);
        std::string content;
        if (*header_value == "eth_getLogs") {
          content =
              R"({"id":1,"jsonrpc":"2.0","result":[{"address":"0x91",
              "blockHash":"0xe8","blockNumber":"0x10","data":"0x0067",
              "logIndex":"0x0","removed":false,
              "topics":["0x4b","0x06e","0x085"],
              "transactionHash":"0x22f7","transactionIndex":"0x0"}]})";
        }
        url_loader_factory_.AddResponse(request.url.spec(), content);
      }));

  // Logs subscription with parameters
  request_payload_json =
      R"({"id":1,"jsonrpc:": "2.0","method":"eth_subscribe",
          "params": ["logs"]})";
  request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  response = CommonRequestOrSendAsync(request_payload.value());

  EXPECT_EQ(response.first, false);
  EXPECT_TRUE(response.second.is_string());
  std::string first_subscription = *response.second.GetIfString();
  browser_task_environment_.FastForwardBy(
      base::Seconds(kLogTrackerDefaultTimeInSeconds));
  EXPECT_TRUE(observer_->MessageEventFired());
  base::Value rv = observer_->GetLastMessage();
  ASSERT_TRUE(rv.is_dict());

  std::string* address = rv.GetDict().FindString("address");
  EXPECT_EQ(*address, "0x91");

  // Make a second subscription
  request_payload_json =
      R"({"id":1,"jsonrpc:": "2.0","method":"eth_subscribe",
          "params": ["logs"]})";
  request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_EQ(response.first, false);
  EXPECT_TRUE(response.second.is_string());
  std::string second_subscription = *response.second.GetIfString();

  // The first unsubscribe should not stop the block tracker
  request_payload_json = base::StringPrintf(R"({"id":1,"jsonrpc:": "2.0",
                              "method":"eth_unsubscribe",
                              "params": ["%s"]})",
                                            first_subscription.c_str());
  request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_TRUE(provider_->eth_logs_tracker_.IsRunning());

  // The second unsubscribe should stop the block tracker
  request_payload_json = base::StringPrintf(R"({"id":1,"jsonrpc:": "2.0",
                              "method":"eth_unsubscribe",
                              "params": ["%s"]})",
                                            second_subscription.c_str());
  request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_FALSE(provider_->eth_logs_tracker_.IsRunning());
}

TEST_F(EthereumProviderImplUnitTest, EthSubscribeLogsFiltered) {
  CreateWallet();
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();

        auto header_value = request.headers.GetHeader("X-Eth-Method");
        ASSERT_TRUE(header_value);

        if (*header_value == "eth_getLogs") {
          const std::optional<base::Value> req_body_payload =
              base::JSONReader::Read(
                  R"({"id":1,"jsonrpc":"2.0","method":"eth_getLogs","params":
[{"address":["0x1111", "0x1112"],"fromBlock":"0x2211","toBlock":"0xab65",
"topics":["0x2edc","0xb832","0x8dc8"]}]})",
                  base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);

          const auto payload = ToValue(request);
          EXPECT_EQ(*payload, req_body_payload.value());
        }
        url_loader_factory_.AddResponse(
            request.url.spec(),
            R"({"id":1,"jsonrpc":"2.0","result":[{"address":"0x91",
                      "blockHash":"0xe8","blockNumber":"0x10","data":"0x0067",
                      "logIndex":"0x0","removed":false,
                      "topics":["0x4b","0x06e","0x085"],
                      "transactionHash":"0x22f7","transactionIndex":"0x0"}]})");
      }));

  // Logs subscription with parameters
  std::string request_payload_json =
      R"({"id":1,"jsonrpc:": "2.0","method":"eth_subscribe",
  "params": ["logs", {"address": ["0x1111", "0x1112"], "fromBlock": "0x2211",
  "toBlock": "0xab65",  "topics":  ["0x2edc", "0xb832", "0x8dc8"]}]})";
  std::optional<base::Value> request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  std::string error_message;
  auto response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_EQ(response.first, false);
  EXPECT_TRUE(response.second.is_string());
  std::string subscription = *response.second.GetIfString();
  browser_task_environment_.FastForwardBy(
      base::Seconds(kLogTrackerDefaultTimeInSeconds));
  EXPECT_TRUE(observer_->MessageEventFired());
  base::Value rv = observer_->GetLastMessage();
  ASSERT_TRUE(rv.is_dict());

  std::string* address = rv.GetDict().FindString("address");
  EXPECT_EQ(*address, "0x91");

  // The first unsubscribe should not stop the block tracker
  request_payload_json = base::StringPrintf(R"({"id":1,"jsonrpc:": "2.0",
                              "method":"eth_unsubscribe",
                              "params": ["%s"]})",
                                            subscription.c_str());
  request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_FALSE(provider_->eth_logs_tracker_.IsRunning());
}

TEST_F(EthereumProviderImplUnitTest, Web3ClientVersion) {
  std::string expected_version = base::StringPrintf(
      "BraveWallet/v%s", version_info::GetBraveChromiumVersionNumber().c_str());
  std::string version;
  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;
  Web3ClientVersion(&version, &error, &error_message);
  EXPECT_EQ(version, expected_version);
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
}

TEST_F(EthereumProviderImplUnitTest, AccountsChangedEventSelectedAccount) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  auto account_1 = GetAccountUtils().EnsureEthAccount(1);
  auto account_2 = GetAccountUtils().EnsureEthAccount(2);

  auto address_0 = base::ToLowerASCII(account_0->address);
  auto address_1 = base::ToLowerASCII(account_1->address);

  GURL url("https://brave.com");
  Navigate(url);

  // Multiple accounts can be returned
  AddEthereumPermission(account_0->account_id);
  // Wait for KeyringService::GetSelectedAccount called by
  // BraveWalletProviderDelegateImpl::GetAllowedAccounts
  browser_task_environment_.RunUntilIdle();
  AddEthereumPermission(account_1->account_id);
  // Wait for KeyringService::GetSelectedAccount called by
  // BraveWalletProviderDelegateImpl::GetAllowedAccounts
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_THAT(observer_->GetLowercaseAccounts(),
              ElementsAre(address_0, address_1));
  observer_->Reset();

  // Changing the selected account only returns that account
  SetSelectedAccount(account_0->account_id);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_THAT(observer_->GetLowercaseAccounts(), ElementsAre(address_0));
  observer_->Reset();

  // Changing to a different allowed account only returns that account
  SetSelectedAccount(account_1->account_id);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_THAT(observer_->GetLowercaseAccounts(), ElementsAre(address_1));
  observer_->Reset();

  // Changing gto a not allowed account returns all allowed accounts
  SetSelectedAccount(account_2->account_id);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_THAT(observer_->GetLowercaseAccounts(),
              ElementsAre(address_0, address_1));
  observer_->Reset();

  // Resetting with multiple accounts works
  ResetEthereumPermission(account_1->account_id);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_THAT(observer_->GetLowercaseAccounts(), ElementsAre(address_0));
  observer_->Reset();
}

TEST_F(EthereumProviderImplUnitTest, GetAllowedAccounts) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  auto account_1 = GetAccountUtils().EnsureEthAccount(1);
  auto account_2 = GetAccountUtils().EnsureEthAccount(2);

  GURL url("https://brave.com");
  Navigate(url);

  auto address_0 = base::ToLowerASCII(account_0->address);
  auto address_1 = base::ToLowerASCII(account_1->address);

  // When nothing is allowed, empty array should be returned
  EXPECT_THAT(GetAllowedAccounts(false), IsEmpty());
  EXPECT_THAT(GetAllowedAccounts(true), IsEmpty());

  // Allowing 1 account should return that account for allowed accounts
  AddEthereumPermission(account_0->account_id);
  EXPECT_THAT(GetAllowedAccounts(false), ElementsAre(address_0));
  EXPECT_THAT(GetAllowedAccounts(true), ElementsAre(address_0));

  // Multiple accounts can be returned
  AddEthereumPermission(account_1->account_id);
  EXPECT_THAT(GetAllowedAccounts(false), ElementsAre(address_0, address_1));
  EXPECT_THAT(GetAllowedAccounts(true), ElementsAre(address_0, address_1));

  // Resetting permissions should return the remaining allowed account
  ResetEthereumPermission(account_1->account_id);
  EXPECT_THAT(GetAllowedAccounts(false), ElementsAre(address_0));
  EXPECT_THAT(GetAllowedAccounts(true), ElementsAre(address_0));

  // Locking the keyring does not return any accounts
  Lock();
  EXPECT_THAT(GetAllowedAccounts(false), IsEmpty());
  EXPECT_THAT(GetAllowedAccounts(true), ElementsAre(address_0));

  // Unlocking restores the accounts that were previously allowed
  Unlock();
  EXPECT_THAT(GetAllowedAccounts(false), ElementsAre(address_0));
  EXPECT_THAT(GetAllowedAccounts(true), ElementsAre(address_0));

  // Selected account should filter the accounts returned
  AddEthereumPermission(account_1->account_id);
  SetSelectedAccount(account_0->account_id);
  EXPECT_THAT(GetAllowedAccounts(false), ElementsAre(address_0));
  EXPECT_THAT(GetAllowedAccounts(true), ElementsAre(address_0));
  SetSelectedAccount(account_1->account_id);
  EXPECT_THAT(GetAllowedAccounts(false), ElementsAre(address_1));
  EXPECT_THAT(GetAllowedAccounts(true), ElementsAre(address_1));
  SetSelectedAccount(account_2->account_id);
  EXPECT_THAT(GetAllowedAccounts(false), ElementsAre(address_0, address_1));
  EXPECT_THAT(GetAllowedAccounts(true), ElementsAre(address_0, address_1));

  // Resetting all accounts should return an empty array again
  ResetEthereumPermission(account_0->account_id);
  ResetEthereumPermission(account_1->account_id);
  EXPECT_THAT(GetAllowedAccounts(false), IsEmpty());
  EXPECT_THAT(GetAllowedAccounts(true), IsEmpty());
}

TEST_F(EthereumProviderImplUnitTest, SignMessageHardware) {
  CreateWallet();
  std::string address = "0xA99D71De40D67394eBe68e4D0265cA6C9D421029";
  auto added_hw_account = AddHardwareAccount(address);
  mojom::EthereumSignatureBytesPtr signature;
  const mojom::EthereumSignatureBytesPtr expected_signature =
      mojom::EthereumSignatureBytes::New(
          std::vector<uint8_t>{1, 2, 3, 4, 5, 6});
  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(added_hw_account->account_id);

  // success
  SignMessageHardware(true, address, "0x1234", expected_signature, "",
                      &signature, &error, &error_message);
  EXPECT_EQ(signature, expected_signature);
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());

  // forwarding errors from javascript
  std::string expected_error = "error text";
  SignMessageHardware(false, address, "0x1234", expected_signature,
                      expected_error, &signature, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kInternalError);
  EXPECT_EQ(error_message, expected_error);

  // user rejected request
  SignMessageHardware(false, address, "0x1234", expected_signature, "",
                      &signature, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
}

TEST_F(EthereumProviderImplUnitTest, SwitchEthereumChain) {
  // CreateWallet();
  CreateBraveWalletTabHelper();
  Navigate(GURL("https://bravesoftware.com"));
  brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);
  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;

  // chain doesn't exist yet
  std::string chain_id = "0x111";
  SwitchEthereumChain(chain_id, std::nullopt, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kUnknownChain);
  EXPECT_EQ(error_message,
            l10n_util::GetStringFUTF8(IDS_WALLET_UNKNOWN_CHAIN,
                                      base::ASCIIToUTF16(chain_id)));
  EXPECT_FALSE(brave_wallet_tab_helper()->IsShowingBubble());

  // already on this chain
  SwitchEthereumChain("0x1", std::nullopt, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  EXPECT_FALSE(brave_wallet_tab_helper()->IsShowingBubble());

  // user rejected
  SwitchEthereumChain("0xaa36a7", false, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  EXPECT_TRUE(brave_wallet_tab_helper()->IsShowingBubble());
  brave_wallet_tab_helper()->CloseBubble();
  EXPECT_FALSE(brave_wallet_tab_helper()->IsShowingBubble());

  // user approved
  SwitchEthereumChain("0xaa36a7", true, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  EXPECT_TRUE(brave_wallet_tab_helper()->IsShowingBubble());
  brave_wallet_tab_helper()->CloseBubble();
  EXPECT_FALSE(brave_wallet_tab_helper()->IsShowingBubble());
  EXPECT_EQ(
      json_rpc_service()->GetChainIdSync(mojom::CoinType::ETH, GetOrigin()),
      "0xaa36a7");

  // one request per origin
  base::RunLoop run_loop;
  provider()->SwitchEthereumChain(
      "0x1",
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            run_loop.Quit();
          }),
      base::Value());
  SwitchEthereumChain("0x1", std::nullopt, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
  json_rpc_service()->NotifySwitchChainRequestProcessed(
      GetPendingSwitchChainRequestId(), true);
  run_loop.Run();
  EXPECT_EQ(
      json_rpc_service()->GetChainIdSync(mojom::CoinType::ETH, GetOrigin()),
      "0x1");
}

TEST_F(EthereumProviderImplUnitTest, AddEthereumChainSwitchesForInnactive) {
  CreateBraveWalletTabHelper();
  Navigate(GURL("https://bravesoftware.com"));
  brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);

  // AddEthereumChain switches for already added networks
  std::string params = R"({"params": [{
        "chainId": "0xaa36a7",
        "chainName": "Sepolia",
        "rpcUrls": ["https://ethereum-sepolia.wallet.brave.com"]
      }]})";
  base::RunLoop run_loop;
  provider()->AddEthereumChain(
      params,
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            mojom::ProviderError error_code;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error_code,
                                &error_message);
            EXPECT_EQ(error_code, mojom::ProviderError::kSuccess);
            EXPECT_TRUE(error_message.empty());
            run_loop.Quit();
          }),
      base::Value());
  EXPECT_TRUE(brave_wallet_tab_helper()->IsShowingBubble());
  json_rpc_service_->NotifySwitchChainRequestProcessed(
      GetPendingSwitchChainRequestId(), true);
  run_loop.Run();
  brave_wallet_tab_helper()->CloseBubble();
  EXPECT_FALSE(brave_wallet_tab_helper()->IsShowingBubble());
  EXPECT_EQ(
      json_rpc_service()->GetChainIdSync(mojom::CoinType::ETH, GetOrigin()),
      "0xaa36a7");
}

TEST_F(EthereumProviderImplUnitTest, AddSuggestToken) {
  CreateBraveWalletTabHelper();
  Navigate(GURL("https://brave.com"));
  brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);

  mojom::BlockchainTokenPtr token = mojom::BlockchainToken::New(
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "BAT", "", false, true,
      false, false, mojom::SPLTokenProgram::kUnsupported, false, false, "BAT",
      18, true, "", "", "0x1", mojom::CoinType::ETH);
  bool approved = false;
  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;
  AddSuggestToken(token.Clone(), true, &approved, &error, &error_message);
  EXPECT_TRUE(approved);
  EXPECT_EQ(mojom::ProviderError::kSuccess, error);
  EXPECT_TRUE(error_message.empty());

  AddSuggestToken(token.Clone(), false, &approved, &error, &error_message);
  EXPECT_FALSE(approved);
  EXPECT_EQ(mojom::ProviderError::kSuccess, error);
  EXPECT_TRUE(error_message.empty());

  AddSuggestToken(nullptr, true, &approved, &error, &error_message);
  EXPECT_FALSE(approved);
  EXPECT_EQ(mojom::ProviderError::kInvalidParams, error);
  EXPECT_FALSE(error_message.empty());
}

TEST_F(EthereumProviderImplUnitTest, GetEncryptionPublicKey) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);

  CreateBraveWalletTabHelper();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(account_0->account_id);
  brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);

  // Happy path
  std::string key;
  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;
  GetEncryptionPublicKey(account_0->address, true, &key, &error,
                         &error_message);
  EXPECT_EQ(key, "GeiNTGIpEKEVFeMBpd3aVs/S2EjoF8FOoichRuqjBg0=");
  EXPECT_EQ(mojom::ProviderError::kSuccess, error);
  EXPECT_TRUE(error_message.empty());

  // Locked should give unauthorized error
  Lock();
  GetEncryptionPublicKey(account_0->address, true, &key, &error,
                         &error_message);
  EXPECT_TRUE(key.empty());
  EXPECT_EQ(mojom::ProviderError::kUnauthorized, error);
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED), error_message);

  // Unlocked and user rejected
  Unlock();
  GetEncryptionPublicKey(account_0->address, false, &key, &error,
                         &error_message);
  EXPECT_TRUE(key.empty());
  EXPECT_EQ(mojom::ProviderError::kUserRejectedRequest, error);
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
            error_message);

  // Address without permissions gives the unathorized error
  auto account_1 = GetAccountUtils().EnsureEthAccount(1);
  GetEncryptionPublicKey(account_1->address, true, &key, &error,
                         &error_message);
  EXPECT_TRUE(key.empty());
  EXPECT_EQ(mojom::ProviderError::kUnauthorized, error);
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED), error_message);

  // Invalid address gives the invalid params error
  GetEncryptionPublicKey("", true, &key, &error, &error_message);
  EXPECT_TRUE(key.empty());
  EXPECT_EQ(mojom::ProviderError::kInvalidParams, error);
  EXPECT_FALSE(error_message.empty());
}

TEST_F(EthereumProviderImplUnitTest, Decrypt) {
  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  auto address_0 = account_0->address;

  CreateBraveWalletTabHelper();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(account_0->account_id);
  brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);

  std::string valid_pi_json =
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"6IWDnjTObWyEB/XpQWT9Rs6CTed24BaA","ephemPublicKey":"XhoADVJjjmI5iUveoJ8sm3v9+wWBwCN6x/6K2tFhdg8=","ciphertext":"lru72L3/fK+X30ZBTxhVmp1YDTb0CZ+NAAxG919PJR9Y0icmpjhEijoASBLB2kR1KfKMtERHxpeCl9XYtmRY87LBRIuRFAmvoA6j0kF4YhDSm4AzMpwQRzvZSIC49rLHJZM1rSDLBMKkFdON0H3D"})";
  std::string empty_message_json =
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"X0HlUQmgWwjiB0794AB4Js/wbzjrM9v9","ephemPublicKey":"nf595GsfgQKpQahDibdvFsxjOCG4j8luJ+fM5WIjoGQ=","ciphertext":"jvRnfKcpv4t1Oghb+q4vqw=="})";

  // Happy path w/ key GeiNTGIpEKEVFeMBpd3aVs/S2EjoF8FOoichRuqjBg0=
  std::string unsafe_message;
  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;
  Decrypt(valid_pi_json, address_0, true, &unsafe_message, &error,
          &error_message);
  EXPECT_EQ(unsafe_message,
            "3."
            "141592653589793238462643383279502884197169399375105820974944592307"
            "816406286208998628034825...");
  EXPECT_EQ(mojom::ProviderError::kSuccess, error);
  EXPECT_TRUE(error_message.empty());

  // Happy path w/ empty message
  Decrypt(empty_message_json, address_0, true, &unsafe_message, &error,
          &error_message);
  EXPECT_TRUE(unsafe_message.empty());
  EXPECT_EQ(mojom::ProviderError::kSuccess, error);
  EXPECT_TRUE(error_message.empty());

  std::vector<std::string> error_cases = {
      // Wrong version
      R"({"version":"x25519-xsalsa20-poly1306","nonce":"6IWDnjTObWyEB/XpQWT9Rs6CTed24BaA","ephemPublicKey":"XhoADVJjjmI5iUveoJ8sm3v9+wWBwCN6x/6K2tFhdg8=","ciphertext":"lru72L3/fK+X30ZBTxhVmp1YDTb0CZ+NAAxG919PJR9Y0icmpjhEijoASBLB2kR1KfKMtERHxpeCl9XYtmRY87LBRIuRFAmvoA6j0kF4YhDSm4AzMpwQRzvZSIC49rLHJZM1rSDLBMKkFdON0H3D"})",
      // Bad nonce
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"5IWDnjTObWyEB/XpQWT9Rs6CTed24BaA","ephemPublicKey":"XhoADVJjjmI5iUveoJ8sm3v9+wWBwCN6x/6K2tFhdg8=","ciphertext":"lru72L3/fK+X30ZBTxhVmp1YDTb0CZ+NAAxG919PJR9Y0icmpjhEijoASBLB2kR1KfKMtERHxpeCl9XYtmRY87LBRIuRFAmvoA6j0kF4YhDSm4AzMpwQRzvZSIC49rLHJZM1rSDLBMKkFdON0H3D"})",
      // Bad ephemeral public key
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"6IWDnjTObWyEB/XpQWT9Rs6CTed24BaA","ephemPublicKey":"YhoADVJjjmI5iUveoJ8sm3v9+wWBwCN6x/6K2tFhdg8=","ciphertext":"lru72L3/fK+X30ZBTxhVmp1YDTb0CZ+NAAxG919PJR9Y0icmpjhEijoASBLB2kR1KfKMtERHxpeCl9XYtmRY87LBRIuRFAmvoA6j0kF4YhDSm4AzMpwQRzvZSIC49rLHJZM1rSDLBMKkFdON0H3D"})",
      // Bad ciphertext
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"6IWDnjTObWyEB/XpQWT9Rs6CTed24BaA","ephemPublicKey":"XhoADVJjjmI5iUveoJ8sm3v9+wWBwCN6x/6K2tFhdg8=","ciphertext":"mru72L3/fK+X30ZBTxhVmp1YDTb0CZ+NAAxG919PJR9Y0icmpjhEijoASBLB2kR1KfKMtERHxpeCl9XYtmRY87LBRIuRFAmvoA6j0kF4YhDSm4AzMpwQRzvZSIC49rLHJZM1rSDLBMKkFdON0H3D"})",
      // Missing version
      R"({"nonce":"6IWDnjTObWyEB/XpQWT9Rs6CTed24BaA","ephemPublicKey":"XhoADVJjjmI5iUveoJ8sm3v9+wWBwCN6x/6K2tFhdg8=","ciphertext":"lru72L3/fK+X30ZBTxhVmp1YDTb0CZ+NAAxG919PJR9Y0icmpjhEijoASBLB2kR1KfKMtERHxpeCl9XYtmRY87LBRIuRFAmvoA6j0kF4YhDSm4AzMpwQRzvZSIC49rLHJZM1rSDLBMKkFdON0H3D"})",
      // Missing nonce
      R"({"version":"x25519-xsalsa20-poly1305","ephemPublicKey":"XhoADVJjjmI5iUveoJ8sm3v9+wWBwCN6x/6K2tFhdg8=","ciphertext":"lru72L3/fK+X30ZBTxhVmp1YDTb0CZ+NAAxG919PJR9Y0icmpjhEijoASBLB2kR1KfKMtERHxpeCl9XYtmRY87LBRIuRFAmvoA6j0kF4YhDSm4AzMpwQRzvZSIC49rLHJZM1rSDLBMKkFdON0H3D"})",
      // Missing ephemeral public key
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"6IWDnjTObWyEB/XpQWT9Rs6CTed24BaA","ciphertext":"lru72L3/fK+X30ZBTxhVmp1YDTb0CZ+NAAxG919PJR9Y0icmpjhEijoASBLB2kR1KfKMtERHxpeCl9XYtmRY87LBRIuRFAmvoA6j0kF4YhDSm4AzMpwQRzvZSIC49rLHJZM1rSDLBMKkFdON0H3D"})",
      // Missing ciphertext
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"6IWDnjTObWyEB/XpQWT9Rs6CTed24BaA","ephemPublicKey":"XhoADVJjjmI5iUveoJ8sm3v9+wWBwCN6x/6K2tFhdg8="})"
      // Wrong JSON
      "[]",
      // Invalid JSON
      "\"Pickle rick"};
  for (auto& error_case : error_cases) {
    Decrypt(error_case, address_0, true, &unsafe_message, &error,
            &error_message);
    EXPECT_TRUE(unsafe_message.empty()) << " case: " << error_case;
    EXPECT_EQ(mojom::ProviderError::kInvalidParams, error)
        << " case: " << error_case;
    EXPECT_FALSE(error_message.empty()) << " case: " << error_case;
  }

  // Locked should give unauthorized error
  Lock();
  Decrypt(valid_pi_json, address_0, true, &unsafe_message, &error,
          &error_message);
  EXPECT_TRUE(unsafe_message.empty());
  EXPECT_EQ(mojom::ProviderError::kUnauthorized, error);
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED), error_message);

  // Unlocked and user rejected
  Unlock();
  Decrypt(valid_pi_json, address_0, false, &unsafe_message, &error,
          &error_message);
  EXPECT_TRUE(unsafe_message.empty());
  EXPECT_EQ(mojom::ProviderError::kUserRejectedRequest, error);
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
            error_message);

  // Address without permissions gives the unauthorized error
  auto address_1 = GetAccountUtils().EnsureEthAccount(1)->address;
  Decrypt(valid_pi_json, address_1, true, &unsafe_message, &error,
          &error_message);
  EXPECT_TRUE(unsafe_message.empty());
  EXPECT_EQ(mojom::ProviderError::kUnauthorized, error);
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED), error_message);

  // Invalid address gives the invalid params error
  Decrypt(valid_pi_json, "", true, &unsafe_message, &error, &error_message);
  EXPECT_TRUE(unsafe_message.empty());
  EXPECT_EQ(mojom::ProviderError::kInvalidParams, error);
  EXPECT_FALSE(error_message.empty());

  // Encrypted string for the message: '\x00\x01\x02' (non-printable)
  Decrypt(
      "0x7b2276657273696f6e223a227832353531392d7873616c736132302d706f6c79313330"
      "35222c226e6f6e6365223a22444d59686b526f712b7a695a7a47366d6142526f48464176"
      "4f33624743456976222c22657068656d5075626c69634b6579223a227a4b634c4f4c5575"
      "7273735a634b377a7a71757062713647566566494a374d6d43656475412f732b577a4d3d"
      "222c2263697068657274657874223a22724964467156436b4e694456504b31366b634b78"
      "50586b424f413d3d227d",
      address_0, true, &unsafe_message, &error, &error_message);
  EXPECT_TRUE(unsafe_message.empty());
  EXPECT_EQ(mojom::ProviderError::kInvalidParams, error);
  EXPECT_FALSE(error_message.empty());
}

TEST_F(EthereumProviderImplUnitTest, RequestEthCoinbase) {
  // Wallet that is not created should return empty base::Value for eth_coinbase
  std::string request_payload_json =
      R"({"id":1,"jsonrpc:": "2.0","method":"eth_coinbase"})";
  std::optional<base::Value> request_payload = base::JSONReader::Read(
      request_payload_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                base::JSONParserOptions::JSON_PARSE_RFC);
  auto response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_EQ(response.first, false);
  EXPECT_EQ(response.second, base::Value());

  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  auto address_0 = base::ToLowerASCII(account_0->address);

  GURL url("https://brave.com");
  Navigate(url);

  // Fresh wallet should return empty base::Value for eth_coinbase
  response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_EQ(response.first, false);
  EXPECT_EQ(response.second, base::Value());

  // Allow 1 account
  AddEthereumPermission(account_0->account_id);
  Lock();

  // eth_coinbase account is empty when locked
  response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_EQ(response.first, false);
  EXPECT_EQ(response.second, base::Value());

  std::vector<std::string> allowed_accounts;
  base::RunLoop run_loop;
  provider()->RequestEthereumPermissions(
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            if (formed_response.GetList().size() != 0) {
              std::string stylesheet = "";
              for (auto& account : formed_response.GetList()) {
                allowed_accounts.push_back(account.GetString());
              }
            }
            run_loop.Quit();
          }),
      base::Value(), "", GetOrigin());
  // Wait for KeyringService::GetSelectedAccount called by
  // BraveWalletProviderDelegateImpl::GetAllowedAccounts
  browser_task_environment_.RunUntilIdle();

  EXPECT_TRUE(keyring_service()->HasPendingUnlockRequest());
  // eth_coinbase account is still empty when locked
  response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_EQ(response.first, false);
  EXPECT_EQ(response.second, base::Value());

  Unlock();
  run_loop.Run();

  // eth_coinbase should now return the account since the account is
  // allowed and the wallet is unlocked
  response = CommonRequestOrSendAsync(request_payload.value());
  EXPECT_FALSE(keyring_service()->HasPendingUnlockRequest());
  EXPECT_EQ(response.first, false);
  EXPECT_EQ(response.second, base::Value(address_0));
}

TEST_F(EthereumProviderImplUnitTest, ProviderResponseFormat) {
  base::Value input = base::test::ParseJson(
      R"({"id":"1","jsonrpc":"2.0","method":"eth_chainId"})");
  const std::string success_rpc_response =
      R"({"jsonrpc":"2.0","id":1,"result":"0x1"})";
  const std::string error_rpc_response =
      R"({"jsonrpc":"2.0","id":1,"error":{"code":-32005,"message":"err"}})";

  // Test provider responses for send, sendAsync are in JsonRpcResponse format.
  // And responses for request are not in JsonRpcResponse format.
  // Success case:
  SetInterceptor(success_rpc_response);
  base::Value::Dict expected_dict =
      base::test::ParseJsonDict(success_rpc_response);
  // Type of id is string in JsonRpcResponse interface.
  // https://docs.metamask.io/guide/ethereum-provider.html#legacy-methods
  expected_dict.Set("id", "1");
  base::Value expected_value = base::Value(std::move(expected_dict));

  auto response = Send("eth_chainId", base::Value());
  EXPECT_FALSE(response.first);
  EXPECT_EQ(response.second, expected_value);

  response = SendAsync(input.Clone());
  EXPECT_FALSE(response.first);
  EXPECT_EQ(response.second, expected_value);

  response = Request(input.Clone());
  EXPECT_FALSE(response.first);
  EXPECT_EQ(response.second, base::Value("0x1"));

  // Error case:
  SetInterceptor(error_rpc_response);
  expected_dict = base::test::ParseJsonDict(error_rpc_response);
  expected_dict.Set("id", "1");
  expected_value = base::Value(std::move(expected_dict));

  response = Send("eth_chainId", base::Value());
  EXPECT_TRUE(response.first);
  EXPECT_EQ(response.second, expected_value);

  response = SendAsync(input.Clone());
  EXPECT_TRUE(response.first);
  EXPECT_EQ(response.second, expected_value);

  response = Request(input.Clone());
  EXPECT_TRUE(response.first);
  EXPECT_EQ(response.second,
            base::test::ParseJson(R"({"code":-32005,"message":"err"})"));

  // Test provider responses for enable are not in JsonRpcResponse format.
  // Error case:
  response = Enable();
  EXPECT_TRUE(response.first);
  EXPECT_EQ(
      response.second,
      base::test::ParseJson(
          R"({"code": -32603,"message": "An internal error has occurred"})"));

  CreateWallet();
  auto account_0 = GetAccountUtils().EnsureEthAccount(0);
  auto address_0 = base::ToLowerASCII(account_0->address);

  GURL url("https://brave.com");
  Navigate(url);

  // Success case:
  AddEthereumPermission(account_0->account_id);
  response = Enable();
  EXPECT_FALSE(response.first);
  base::Value::List expected_list;
  expected_list.Append(base::Value(address_0));
  EXPECT_EQ(response.second, base::Value(std::move(expected_list)));
}

}  // namespace brave_wallet
