/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#include "brave/components/version_info/version_info.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/permission_request_manager.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_contents_factory.h"
#include "content/test/test_web_contents.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

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
  const base::Value* code = formed_response.FindKey("code");
  if (code) {
    *error = static_cast<mojom::ProviderError>(code->GetInt());
  }
  const base::Value* message = formed_response.FindKey("message");
  if (message) {
    *error_message = message->GetString();
  }
}

const char kMnemonic1[] =
    "divide cruise upon flag harsh carbon filter merit once advice bright "
    "drive";

void ValidateErrorCode(BraveWalletProviderImpl* provider,
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
            mojom::ProviderError error;
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

}  // namespace

class TestEventsListener : public brave_wallet::mojom::EventsListener {
 public:
  TestEventsListener() {}

  void ChainChangedEvent(const std::string& chain_id) override {
    chain_id_ = chain_id;
    chain_changed_fired_ = true;
  }

  void AccountsChangedEvent(const std::vector<std::string>& accounts) override {
    accounts_ = accounts;
    accounts_changed_fired_ = true;
  }

  bool ChainChangedFired() const {
    base::RunLoop().RunUntilIdle();
    return chain_changed_fired_;
  }

  bool AccountsChangedFired() const {
    base::RunLoop().RunUntilIdle();
    return accounts_changed_fired_;
  }

  std::string GetChainId() const {
    base::RunLoop().RunUntilIdle();
    return chain_id_;
  }

  std::vector<std::string> GetAccounts() const {
    base::RunLoop().RunUntilIdle();
    return accounts_;
  }

  mojo::PendingRemote<brave_wallet::mojom::EventsListener> GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  void Reset() {
    chain_id_.clear();
    accounts_.clear();
    chain_changed_fired_ = false;
    accounts_changed_fired_ = false;
    EXPECT_FALSE(ChainChangedFired());
    EXPECT_FALSE(AccountsChangedFired());
  }

  bool chain_changed_fired_ = false;
  bool accounts_changed_fired_ = false;
  std::vector<std::string> accounts_;
  std::string chain_id_;

 private:
  mojo::Receiver<brave_wallet::mojom::EventsListener> observer_receiver_{this};
};

class BraveWalletProviderImplUnitTest : public testing::Test {
 public:
  BraveWalletProviderImplUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void TearDown() override {
    provider_.reset();
    web_contents_.reset();
  }

  void SetUp() override {
    web_contents_ =
        content::TestWebContents::Create(browser_context(), nullptr);
    permissions::PermissionRequestManager::CreateForWebContents(web_contents());
    json_rpc_service_ =
        JsonRpcServiceFactory::GetServiceForContext(browser_context());
    json_rpc_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
    SetNetwork("0x1");
    keyring_service_ =
        KeyringServiceFactory::GetServiceForContext(browser_context());
    asset_ratio_service_ =
        AssetRatioServiceFactory::GetServiceForContext(browser_context());
    asset_ratio_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
    tx_service_ = TxServiceFactory::GetServiceForContext(browser_context());
    brave_wallet_service_ =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            browser_context());

    provider_ = std::make_unique<BraveWalletProviderImpl>(
        host_content_settings_map(), json_rpc_service(), tx_service(),
        keyring_service(), brave_wallet_service_,
        std::make_unique<brave_wallet::BraveWalletProviderDelegateImpl>(
            web_contents(), web_contents()->GetMainFrame()),
        prefs());

    observer_.reset(new TestEventsListener());
    provider_->Init(observer_->GetReceiver());
  }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetNetwork(const std::string& chain_id) {
    base::RunLoop run_loop;
    json_rpc_service_->SetNetwork(
        chain_id, mojom::CoinType::ETH,
        base::BindLambdaForTesting([&run_loop](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void CreateWallet() {
    base::RunLoop run_loop;
    keyring_service_->CreateWallet(
        "brave",
        base::BindLambdaForTesting([&run_loop](const std::string& mnemonic) {
          EXPECT_FALSE(mnemonic.empty());
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void RestoreWallet(const std::string& mnemonic,
                     const std::string& password,
                     bool is_legacy_brave_wallet) {
    base::RunLoop run_loop;
    keyring_service_->RestoreWallet(
        mnemonic, password, is_legacy_brave_wallet,
        base::BindLambdaForTesting([&](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void AddAccount() {
    base::RunLoop run_loop;
    keyring_service_->AddAccount(
        "New Account", mojom::CoinType::ETH,
        base::BindLambdaForTesting([&run_loop](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }
  void AddHardwareAccount(const std::string& address) {
    std::vector<mojom::HardwareWalletAccountPtr> hw_accounts;
    hw_accounts.push_back(mojom::HardwareWalletAccount::New(
        address, "m/44'/60'/1'/0/0", "name 1", "Ledger", "device1",
        mojom::CoinType::ETH));

    keyring_service_->AddHardwareAccounts(std::move(hw_accounts));
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

  void SetSelectedAccount(const std::string& address, mojom::CoinType coin) {
    base::RunLoop run_loop;
    keyring_service_->SetSelectedAccount(
        address, coin, base::BindLambdaForTesting([&](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  std::vector<std::string> GetAllowedAccounts(
      bool include_accounts_when_locked) {
    std::vector<std::string> allowed_accounts;
    base::RunLoop run_loop;
    provider()->GetAllowedAccounts(
        include_accounts_when_locked,
        base::BindLambdaForTesting([&](const std::vector<std::string>& accounts,
                                       mojom::ProviderError error,
                                       const std::string& error_message) {
          allowed_accounts = accounts;
          EXPECT_EQ(error, mojom::ProviderError::kSuccess);
          EXPECT_TRUE(error_message.empty());
          run_loop.Quit();
        }));
    run_loop.Run();
    return allowed_accounts;
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

  ~BraveWalletProviderImplUnitTest() override = default;

  content::TestWebContents* web_contents() { return web_contents_.get(); }
  TxService* tx_service() { return tx_service_; }
  JsonRpcService* json_rpc_service() { return json_rpc_service_; }
  KeyringService* keyring_service() { return keyring_service_; }
  BraveWalletProviderImpl* provider() { return provider_.get(); }
  std::string from(size_t from_index = 0) {
    CHECK(!keyring_service_->IsLocked());
    return keyring_service()
        ->GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId)
        ->GetAddress(from_index);
  }

  content::BrowserContext* browser_context() { return &profile_; }
  PrefService* prefs() { return profile_.GetPrefs(); }
  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(&profile_);
  }

  void Navigate(const GURL& url) { web_contents()->NavigateAndCommit(url); }

  url::Origin GetOrigin() {
    return web_contents()->GetMainFrame()->GetLastCommittedOrigin();
  }

  void CreateBraveWalletTabHelper() {
    brave_wallet::BraveWalletTabHelper::CreateForWebContents(
        web_contents_.get());
  }
  brave_wallet::BraveWalletTabHelper* brave_wallet_tab_helper() {
    return brave_wallet::BraveWalletTabHelper::FromWebContents(
        web_contents_.get());
  }

  void AddEthereumPermission(const url::Origin& origin, size_t from_index = 0) {
    AddEthereumPermission(GetOrigin(), from(from_index));
  }
  void AddEthereumPermission(const url::Origin& origin,
                             const std::string& address) {
    base::RunLoop run_loop;
    brave_wallet_service_->AddEthereumPermission(
        origin, address, base::BindLambdaForTesting([&](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void ResetEthereumPermission(const url::Origin& origin,
                               size_t from_index = 0) {
    permissions::BraveEthereumPermissionContext::ResetEthereumPermission(
        browser_context(), origin, from(from_index));
  }

  void Web3ClientVersion(std::string* version,
                         mojom::ProviderError* error_out,
                         std::string* error_message_out) {
    if (!version || !error_out || !error_message_out)
      return;
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

  void SignMessageHardware(bool user_approved,
                           const std::string& address,
                           const std::string& message,
                           const std::string& hardware_signature,
                           const std::string& error_in,
                           std::string* signature_out,
                           mojom::ProviderError* error_out,
                           std::string* error_message_out) {
    if (!signature_out || !error_out || !error_message_out)
      return;

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
    // Wait for BraveWalletProviderImpl::ContinueSignMessage
    browser_task_environment_.RunUntilIdle();
    brave_wallet_service_->NotifySignMessageHardwareRequestProcessed(
        user_approved, provider()->sign_message_id_ - 1, hardware_signature,
        error_in);
    run_loop.Run();
  }

  void SignMessage(absl::optional<bool> user_approved,
                   const std::string& address,
                   const std::string& message,
                   std::string* signature_out,
                   mojom::ProviderError* error_out,
                   std::string* error_message_out) {
    if (!signature_out || !error_out || !error_message_out)
      return;

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
    // Wait for BraveWalletProviderImpl::ContinueSignMessage
    browser_task_environment_.RunUntilIdle();
    if (user_approved)
      brave_wallet_service_->NotifySignMessageRequestProcessed(
          *user_approved, provider()->sign_message_id_ - 1);
    run_loop.Run();
  }

  void RecoverAddress(const std::string& message,
                      const std::string& signature,
                      std::string* address_out,
                      mojom::ProviderError* error_out,
                      std::string* error_message_out) {
    if (!address_out || !error_out || !error_message_out)
      return;

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

  void SignTypedMessage(absl::optional<bool> user_approved,
                        const std::string& address,
                        const std::string& message,
                        const std::vector<uint8_t>& domain_hash,
                        const std::vector<uint8_t>& primary_hash,
                        base::Value&& domain,
                        std::string* signature_out,
                        mojom::ProviderError* error_out,
                        std::string* error_message_out) {
    if (!signature_out || !error_out || !error_message_out)
      return;

    base::RunLoop run_loop;
    provider()->SignTypedMessage(
        address, message, domain_hash, primary_hash, std::move(domain),
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
    // Wait for BraveWalletProviderImpl::ContinueSignMessage
    browser_task_environment_.RunUntilIdle();
    if (user_approved)
      brave_wallet_service_->NotifySignMessageRequestProcessed(
          *user_approved, provider()->sign_message_id_ - 1);
    run_loop.Run();
  }

  // current request id will be returned
  int SignMessageRequest(const std::string& address,
                         const std::string& message) {
    provider()->SignMessage(address, message, base::DoNothing(), base::Value());
    base::RunLoop().RunUntilIdle();
    return provider()->sign_message_id_ - 1;
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

  std::vector<mojom::SignMessageRequestPtr> GetPendingSignMessageRequests()
      const {
    base::RunLoop run_loop;
    std::vector<mojom::SignMessageRequestPtr> requests_out;
    brave_wallet_service_->GetPendingSignMessageRequests(
        base::BindLambdaForTesting(
            [&](std::vector<mojom::SignMessageRequestPtr> requests) {
              for (const auto& request : requests)
                requests_out.push_back(request.Clone());
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
              for (const auto& request : requests)
                requests_out.push_back(request.Clone());
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
          for (const auto& request : requests)
            requests_out.push_back(request.Clone());
          run_loop.Quit();
        }));
    run_loop.Run();
    return requests_out;
  }

  std::vector<std::string> GetAddresses() {
    std::vector<std::string> result;
    base::RunLoop run_loop;
    keyring_service_->GetKeyringInfo(
        brave_wallet::mojom::kDefaultKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          for (size_t i = 0; i < keyring_info->account_infos.size(); ++i) {
            result.push_back(keyring_info->account_infos[i]->address);
          }
          run_loop.Quit();
        }));
    run_loop.Run();
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

  std::vector<mojom::TransactionInfoPtr> GetAllTransactionInfo() {
    std::vector<mojom::TransactionInfoPtr> transaction_infos;
    base::RunLoop run_loop;
    tx_service()->GetAllTransactionInfo(
        mojom::CoinType::ETH, from(),
        base::BindLambdaForTesting(
            [&](std::vector<mojom::TransactionInfoPtr> v) {
              transaction_infos = std::move(v);
              run_loop.Quit();
            }));
    run_loop.Run();
    return transaction_infos;
  }

  bool ApproveTransaction(const std::string& tx_meta_id,
                          mojom::ProviderError* error_out,
                          std::string* error_message_out) {
    bool success;
    base::RunLoop run_loop;
    tx_service()->ApproveTransaction(
        mojom::CoinType::ETH, tx_meta_id,
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
                           absl::optional<bool> user_approved,
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
    if (user_approved)
      json_rpc_service_->NotifySwitchChainRequestProcessed(*user_approved,
                                                           GetOrigin());
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
          mojom::ProviderError error;
          std::string error_message;
          GetErrorCodeMessage(std::move(formed_response), &error,
                              &error_message);
          *error_out = error;
          *error_message_out = error_message;
          run_loop.Quit();
        }),
        base::Value());
    auto requests = GetPendingGetEncryptionPublicKeyRequests();
    if (requests.size() > 0) {
      ASSERT_EQ(requests.size(), 1u);
      EXPECT_EQ(requests[0]->origin_info, MakeOriginInfo(GetOrigin()));
      EXPECT_EQ(requests[0]->address, address);
      EXPECT_TRUE(brave_wallet_tab_helper()->IsShowingBubble());
      brave_wallet_service_->NotifyGetPublicKeyRequestProcessed(approved,
                                                                GetOrigin());
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
          mojom::ProviderError error;
          std::string error_message;
          GetErrorCodeMessage(std::move(formed_response), &error,
                              &error_message);
          *error_out = error;
          *error_message_out = error_message;
          run_loop.Quit();
        }),
        base::Value());
    // The request is not immediately added, needs sanitization first
    base::RunLoop().RunUntilIdle();
    auto requests = GetPendingDecryptRequests();
    if (requests.size() > 0) {
      ASSERT_EQ(requests.size(), 1u);
      EXPECT_EQ(requests[0]->origin_info, MakeOriginInfo(GetOrigin()));
      EXPECT_EQ(requests[0]->address, address);
      EXPECT_TRUE(brave_wallet_tab_helper()->IsShowingBubble());
      brave_wallet_service_->NotifyDecryptRequestProcessed(approved,
                                                           GetOrigin());
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
              for (const auto& request : requests)
                requests_out.push_back(request.Clone());
              run_loop.Quit();
            }));
    run_loop.Run();
    return requests_out;
  }

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  std::unique_ptr<TestEventsListener> observer_;

 private:
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  content::TestWebContentsFactory factory_;
  raw_ptr<TxService> tx_service_;
  raw_ptr<AssetRatioService> asset_ratio_service_;
  std::unique_ptr<content::TestWebContents> web_contents_;
  std::unique_ptr<BraveWalletProviderImpl> provider_;
  network::TestURLLoaderFactory url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  base::ScopedTempDir temp_dir_;
  TestingProfile profile_;
};

TEST_F(BraveWalletProviderImplUnitTest, ValidateBrokenPayloads) {
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

TEST_F(BraveWalletProviderImplUnitTest, EmptyDelegate) {
  BraveWalletProviderImpl provider_impl(
      host_content_settings_map(), json_rpc_service(), tx_service(),
      keyring_service(), brave_wallet_service_, nullptr, prefs());
  ValidateErrorCode(&provider_impl,
                    R"({"params": [{
        "chainId": "0x111",
        "chainName": "Binance1 Smart Chain",
        "rpcUrls": ["https://bsc-dataseed.binance.org/"]
      }]})",
                    mojom::ProviderError::kInternalError);
}

TEST_F(BraveWalletProviderImplUnitTest, OnAddEthereumChain) {
  GURL url("https://brave.com");
  Navigate(url);
  base::RunLoop run_loop;
  provider()->AddEthereumChain(
      R"({"params": [{
        "chainId": "0x111",
        "chainName": "Binance1 Smart Chain",
        "rpcUrls": ["https://bsc-dataseed.binance.org/"],
      },]})",
      base::BindLambdaForTesting(
          [&run_loop](base::Value id, base::Value formed_response,
                      const bool reject,
                      const std::string& first_allowed_account,
                      const bool update_bind_js_properties) {
            mojom::ProviderError error;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
            EXPECT_EQ(error_message, "test");
            run_loop.Quit();
          }),
      base::Value());
  provider()->OnAddEthereumChain(
      "0x111", mojom::ProviderError::kUserRejectedRequest, "test");
  run_loop.Run();

  provider()->AddEthereumChain(
      R"({"params": [{
        "chainId": "0x111",
        "chainName": "Binance1 Smart Chain",
        "rpcUrls": ["https://bsc-dataseed.binance.org/"],
      },]})",
      base::BindLambdaForTesting(
          [&run_loop](base::Value id, base::Value formed_response,
                      const bool reject,
                      const std::string& first_allowed_account,
                      const bool update_bind_js_properties) {
            mojom::ProviderError error;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
            EXPECT_EQ(error_message, "response");
            run_loop.Quit();
          }),
      base::Value());
  provider()->OnAddEthereumChain(
      "0x111", mojom::ProviderError::kUserRejectedRequest, "response");
}

TEST_F(BraveWalletProviderImplUnitTest,
       OnAddEthereumChainRequestCompletedError) {
  GURL url("https://brave.com");
  Navigate(url);
  base::RunLoop run_loop;
  size_t callback_called = 0;
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
            mojom::ProviderError error;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
            EXPECT_EQ(error_message, "test message");
            ++callback_called;
            run_loop.Quit();
          }),
      base::Value());
  provider()->OnAddEthereumChainRequestCompleted("0x111", "test message");
  provider()->OnAddEthereumChainRequestCompleted("0x111", "test message");
  run_loop.Run();
  EXPECT_EQ(callback_called, 1u);
}

TEST_F(BraveWalletProviderImplUnitTest, AddAndApproveTransaction) {
  bool callback_called = false;
  std::string tx_hash;
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin());

  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      from() +
      "\",\"gasPrice\":\"0x09184e72a000\","
      "\"gas\":\"0x0974\",\"to\":"
      "\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x016345785d8a0000\"}]}";
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& response = value_with_error.value;
  provider()->Request(
      response->Clone(),
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            if (formed_response.type() == base::Value::Type::STRING) {
              tx_hash = formed_response.GetString();
            }
            mojom::ProviderError error;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kSuccess);
            EXPECT_FALSE(tx_hash.empty());
            EXPECT_TRUE(error_message.empty());
            callback_called = true;
          }));
  base::RunLoop().RunUntilIdle();
  std::vector<mojom::TransactionInfoPtr> infos = GetAllTransactionInfo();
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(infos[0]->from_address, from()));
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Unapproved);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);

  EXPECT_EQ(*GetTransactionInfo(infos[0]->id), *infos[0]);
  EXPECT_TRUE(GetTransactionInfo("unknown_id").is_null());

  // Set an interceptor and just fake a common repsonse for
  // eth_getTransactionCount and eth_sendRawTransaction
  SetInterceptor("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0\"}");

  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;

  EXPECT_TRUE(ApproveTransaction(infos[0]->id, &error, &error_message));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  EXPECT_TRUE(callback_called);
  infos = GetAllTransactionInfo();
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(infos[0]->from_address, from()));
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Submitted);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);
}

TEST_F(BraveWalletProviderImplUnitTest, AddAndApproveTransactionError) {
  // We don't need to check every error type since that is checked by
  // eth_tx_manager_unittest but make sure an error type is handled
  // correctly.
  bool callback_called = false;
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin());
  // Bad address
  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      from() +
      "\",\"gasPrice\":\"0x09184e72a000\","
      "\"gas\":\"0x0974\",\"to\":\"0xbe8\","
      "\"value\":\"0x016345785d8a0000\"}]}";
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& response = value_with_error.value;
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
            mojom::ProviderError error;
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

TEST_F(BraveWalletProviderImplUnitTest, AddAndApproveTransactionNoPermission) {
  bool callback_called = false;
  CreateWallet();
  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      from() +
      "\",\"gasPrice\":\"0x09184e72a000\","
      "\"gas\":\"0x0974\",\"to\":"
      "\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x016345785d8a0000\"}]}";
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& response = value_with_error.value;
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
            mojom::ProviderError error;
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

TEST_F(BraveWalletProviderImplUnitTest, AddAndApprove1559Transaction) {
  bool callback_called = false;
  std::string tx_hash;
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin());
  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      from() +
      "\",\"maxFeePerGas\":\"0x1\",\"maxPriorityFeePerGas\":\"0x1\","
      "\"gas\":\"0x1\",\"to\":\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x00\"}]}";
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& response = value_with_error.value;
  provider()->Request(
      response->Clone(),
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            if (formed_response.type() == base::Value::Type::STRING) {
              tx_hash = formed_response.GetString();
            }
            mojom::ProviderError error;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kSuccess);
            EXPECT_FALSE(tx_hash.empty());
            EXPECT_TRUE(error_message.empty());
            callback_called = true;
          }));
  browser_task_environment_.RunUntilIdle();
  std::vector<mojom::TransactionInfoPtr> infos = GetAllTransactionInfo();
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(infos[0]->from_address, from()));
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Unapproved);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);

  // Set an interceptor and just fake a common repsonse for
  // eth_getTransactionCount and eth_sendRawTransaction
  SetInterceptor("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0\"}");

  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;

  EXPECT_TRUE(ApproveTransaction(infos[0]->id, &error, &error_message));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  EXPECT_TRUE(callback_called);
  infos = GetAllTransactionInfo();
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(infos[0]->from_address, from()));
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Submitted);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);
}

TEST_F(BraveWalletProviderImplUnitTest, AddAndApprove1559TransactionNoChainId) {
  std::string tx_hash;
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  SetNetwork("0x4");
  // Wait for EthTxStateManager::ChainChangedEvent to be called.
  browser_task_environment_.RunUntilIdle();

  AddEthereumPermission(GetOrigin());
  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      from() +
      "\",\"maxFeePerGas\":\"0x1\",\"maxPriorityFeePerGas\":\"0x1\","
      "\"gas\":\"0x1\",\"to\":\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x00\"}]}";
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& response = value_with_error.value;
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
            mojom::ProviderError error;
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
            mojom::ProviderError error;
            std::string error_message;
            GetErrorCodeMessage(std::move(formed_response), &error,
                                &error_message);
            EXPECT_EQ(error, mojom::ProviderError::kSuccess);
            EXPECT_FALSE(tx_hash.empty());
            EXPECT_TRUE(error_message.empty());
          }));
  browser_task_environment_.RunUntilIdle();
  std::vector<mojom::TransactionInfoPtr> infos = GetAllTransactionInfo();
  ASSERT_EQ(infos.size(), 2UL);
  ASSERT_TRUE(infos[0]->tx_data_union->is_eth_tx_data_1559());
  EXPECT_EQ(infos[0]->tx_data_union->get_eth_tx_data_1559()->chain_id, "0x4");
  EXPECT_EQ(infos[1]->tx_data_union->get_eth_tx_data_1559()->chain_id, "0x4");
}

TEST_F(BraveWalletProviderImplUnitTest, AddAndApprove1559TransactionError) {
  // We don't need to check every error type since that is checked by
  // eth_tx_manager_unittest but make sure an error type is handled
  // correctly.
  bool callback_called = false;
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin());
  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      from() +
      "\",\"maxFeePerGas\":\"0x0\",\"maxPriorityFeePerGas\":\"0x0\","
      "\"gasPrice\":\"0x01\", "
      "\"gas\":\"0x00\",\"to\":\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x00\"}]}";
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& response = value_with_error.value;
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
            mojom::ProviderError error;
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

TEST_F(BraveWalletProviderImplUnitTest,
       AddAndApprove1559TransactionNoPermission) {
  bool callback_called = false;
  CreateWallet();
  std::string normalized_json_request =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\","
      "\"params\":[{\"from\":\"" +
      from() +
      "\",\"maxFeePerGas\":\"0x0\",\"maxPriorityFeePerGas\":\"0x0\","
      "\"gas\":\"0x00\",\"to\":\"0xbe862ad9abfe6f22bcb087716c7d89a26051f74c\","
      "\"value\":\"0x00\"}]}";
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          normalized_json_request, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& response = value_with_error.value;
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
            mojom::ProviderError error;
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

TEST_F(BraveWalletProviderImplUnitTest, RequestEthereumPermissionNotNewSetup) {
  bool new_setup_callback_called = false;
  SetCallbackForNewSetupNeededForTesting(
      base::BindLambdaForTesting([&]() { new_setup_callback_called = true; }));
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin());
  base::RunLoop run_loop;
  EXPECT_EQ(RequestEthereumPermissions(), std::vector<std::string>{from(0)});
  // Make sure even with a delay the new setup callback is not called.
  browser_task_environment_.RunUntilIdle();
  EXPECT_FALSE(new_setup_callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest,
       RequestEthereumPermissionsNoPermission) {
  bool new_setup_callback_called = false;
  SetCallbackForNewSetupNeededForTesting(
      base::BindLambdaForTesting([&]() { new_setup_callback_called = true; }));
  bool permission_callback_called = false;
  CreateWallet();
  AddAccount();
  provider()->RequestEthereumPermissions(
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            mojom::ProviderError error;
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

TEST_F(BraveWalletProviderImplUnitTest, RequestEthereumPermissionsNoWallet) {
  bool new_setup_callback_called = false;
  SetCallbackForNewSetupNeededForTesting(
      base::BindLambdaForTesting([&]() { new_setup_callback_called = true; }));
  base::RunLoop run_loop;
  provider()->RequestEthereumPermissions(
      base::BindLambdaForTesting(
          [&](base::Value id, base::Value formed_response, const bool reject,
              const std::string& first_allowed_account,
              const bool update_bind_js_properties) {
            mojom::ProviderError error;
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
}

TEST_F(BraveWalletProviderImplUnitTest,
       RequestEthereumPermissionsWithAccounts) {
  CreateWallet();
  AddAccount();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);

  // Allowing 1 account should return that account for allowed accounts
  AddEthereumPermission(GetOrigin(), 0);
  EXPECT_EQ(RequestEthereumPermissions(), std::vector<std::string>{from(0)});

  // Multiple accounts can be returned
  AddEthereumPermission(GetOrigin(), 1);
  EXPECT_EQ(RequestEthereumPermissions(),
            (std::vector<std::string>{from(0), from(1)}));

  // Resetting permissions should return the remaining allowed account
  ResetEthereumPermission(GetOrigin(), 1);
  EXPECT_EQ(RequestEthereumPermissions(), std::vector<std::string>{from(0)});

  // Selected account should filter the accounts returned
  AddEthereumPermission(GetOrigin(), 1);
  SetSelectedAccount(from(0), mojom::CoinType::ETH);
  EXPECT_EQ(RequestEthereumPermissions(), std::vector<std::string>{from(0)});
  SetSelectedAccount(from(1), mojom::CoinType::ETH);
  EXPECT_EQ(RequestEthereumPermissions(), std::vector<std::string>{from(1)});
  SetSelectedAccount(from(2), mojom::CoinType::ETH);
  EXPECT_EQ(RequestEthereumPermissions(),
            (std::vector<std::string>{from(0), from(1)}));
}

TEST_F(BraveWalletProviderImplUnitTest, RequestEthereumPermissionsLocked) {
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);

  std::string account0 = from(0);

  // Allowing 1 account should return that account for allowed accounts
  AddEthereumPermission(GetOrigin(), 0);
  Lock();
  // Allowed accounts is empty when locked
  EXPECT_EQ(GetAllowedAccounts(false), std::vector<std::string>());
  EXPECT_EQ(GetAllowedAccounts(true), std::vector<std::string>{account0});
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

  EXPECT_TRUE(keyring_service()->HasPendingUnlockRequest());
  // Allowed accounts is still empty when locked
  EXPECT_EQ(GetAllowedAccounts(false), std::vector<std::string>());
  EXPECT_EQ(GetAllowedAccounts(true), std::vector<std::string>{account0});
  Unlock();
  run_loop.Run();

  EXPECT_FALSE(keyring_service()->HasPendingUnlockRequest());
  EXPECT_EQ(allowed_accounts, std::vector<std::string>{account0});
}

TEST_F(BraveWalletProviderImplUnitTest, SignMessage) {
  CreateWallet();
  AddAccount();
  std::string signature;
  mojom::ProviderError error;
  std::string error_message;
  SignMessage(absl::nullopt, "1234", "0x1234", &signature, &error,
              &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  SignMessage(absl::nullopt, "0x12345678", "0x1234", &signature, &error,
              &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  const std::string address = "0x1234567890123456789012345678901234567890";
  SignMessage(absl::nullopt, address, "0x1234", &signature, &error,
              &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
  EXPECT_EQ(error_message,
            l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                      base::ASCIIToUTF16(address)));

  // No permission
  const std::vector<std::string> addresses = GetAddresses();
  ASSERT_FALSE(address.empty());
  SignMessage(absl::nullopt, addresses[0], "0x1234", &signature, &error,
              &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
  EXPECT_EQ(error_message,
            l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                      base::ASCIIToUTF16(addresses[0])));
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin());
  SignMessage(true, addresses[0], "0x1234", &signature, &error, &error_message);

  EXPECT_FALSE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());

  // User reject request
  SignMessage(false, addresses[0], "0x1234", &signature, &error,
              &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  keyring_service()->Lock();

  // nullopt for the first param here because we don't AddSignMessageRequest
  // whent here are no accounts returned.
  SignMessage(absl::nullopt, addresses[0], "0x1234", &signature, &error,
              &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
  EXPECT_EQ(error_message,
            l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                      base::ASCIIToUTF16(addresses[0])));
}

TEST_F(BraveWalletProviderImplUnitTest, RecoverAddress) {
  CreateWallet();
  AddAccount();

  std::string signature;
  mojom::ProviderError error;
  std::string error_message;
  const std::vector<std::string> addresses = GetAddresses();

  std::string message = "0x68656c6c6f20776f726c64";
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin());
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

TEST_F(BraveWalletProviderImplUnitTest, SignTypedMessage) {
  EXPECT_EQ(json_rpc_service()->GetChainId(mojom::CoinType::ETH), "0x1");
  CreateWallet();
  AddAccount();
  std::string signature;
  mojom::ProviderError error;
  std::string error_message;
  base::Value domain(base::Value::Type::DICTIONARY);
  std::vector<uint8_t> domain_hash = DecodeHexHash(
      "f2cee375fa42b42143804025fc449deafd50cc031ca257e0b194a650a912090f");
  std::vector<uint8_t> primary_hash = DecodeHexHash(
      "c52c0ee5d84264471806290a3f2c4cecfc5490626bf912d01f240d7a274b371e");
  domain.SetIntKey("chainId", 1);
  SignTypedMessage(absl::nullopt, "1234", "{...}", domain_hash, primary_hash,
                   domain.Clone(), &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  SignTypedMessage(absl::nullopt, "0x12345678", "{...}", domain_hash,
                   primary_hash, domain.Clone(), &signature, &error,
                   &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  const std::string address = "0x1234567890123456789012345678901234567890";
  // domain not dict
  SignTypedMessage(absl::nullopt, address, "{...}", domain_hash, primary_hash,
                   base::Value("not dict"), &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // not valid domain hash
  SignTypedMessage(absl::nullopt, address, "{...}", {}, primary_hash,
                   domain.Clone(), &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // not valid primary hash
  SignTypedMessage(absl::nullopt, address, "{...}", domain_hash, {},
                   domain.Clone(), &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  domain.SetIntKey("chainId", 4);
  std::string chain_id = "0x4";
  // not active network
  SignTypedMessage(absl::nullopt, address, "{...}", domain_hash, primary_hash,
                   domain.Clone(), &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInternalError);
  EXPECT_EQ(error_message,
            l10n_util::GetStringFUTF8(
                IDS_BRAVE_WALLET_SIGN_TYPED_MESSAGE_CHAIN_ID_MISMATCH,
                base::ASCIIToUTF16(chain_id)));
  domain.SetIntKey("chainId", 1);

  SignTypedMessage(absl::nullopt, address, "{...}", domain_hash, primary_hash,
                   domain.Clone(), &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
  EXPECT_EQ(error_message,
            l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                      base::ASCIIToUTF16(address)));

  // No permission
  const std::vector<std::string> addresses = GetAddresses();
  ASSERT_FALSE(address.empty());
  SignTypedMessage(absl::nullopt, addresses[0], "{...}", domain_hash,
                   primary_hash, domain.Clone(), &signature, &error,
                   &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
  EXPECT_EQ(error_message,
            l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                      base::ASCIIToUTF16(addresses[0])));
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin());
  SignTypedMessage(true, addresses[0], "{...}", domain_hash, primary_hash,
                   domain.Clone(), &signature, &error, &error_message);

  EXPECT_FALSE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());

  // User reject request
  SignTypedMessage(false, addresses[0], "{...}", domain_hash, primary_hash,
                   domain.Clone(), &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  // not valid eip712 domain hash
  SignTypedMessage(absl::nullopt, address, "{...}", DecodeHexHash("brave"),
                   primary_hash, domain.Clone(), &signature, &error,
                   &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  // not valid eip712 primary hash
  SignTypedMessage(absl::nullopt, address, "{...}", domain_hash,
                   DecodeHexHash("primary"), domain.Clone(), &signature, &error,
                   &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  keyring_service()->Lock();

  // nullopt for the first param here because we don't AddSignMessageRequest
  // whent here are no accounts returned.
  SignTypedMessage(absl::nullopt, addresses[0], "{...}", domain_hash,
                   primary_hash, domain.Clone(), &signature, &error,
                   &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUnauthorized);
  EXPECT_EQ(error_message,
            l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                      base::ASCIIToUTF16(addresses[0])));
}

TEST_F(BraveWalletProviderImplUnitTest, SignMessageRequestQueue) {
  CreateWallet();
  AddAccount();
  std::string hardware = "0xA99D71De40D67394eBe68e4D0265cA6C9D421029";
  AddHardwareAccount(hardware);
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin());
  AddEthereumPermission(GetOrigin(), hardware);
  const std::vector<std::string> addresses = GetAddresses();

  const std::string message1 = "0x68656c6c6f20776f726c64";
  const std::string message2 = "0x4120756e69636f646520c68e20737472696e6720c3b1";
  const std::string message3 = "0xbeef03";
  int id1 = SignMessageRequest(addresses[0], message1);
  int id2 = SignMessageRequest(addresses[0], message2);
  int id3 = SignMessageRequest(hardware, message3);

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
  EXPECT_EQ(GetSignMessageQueueFront()->message, message1_in_queue);
  {
    auto queue = GetPendingSignMessageRequests();
    ASSERT_EQ(queue.size(), 3u);
    EXPECT_EQ(queue[0]->id, id1);
    EXPECT_EQ(queue[0]->message, message1_in_queue);
    EXPECT_EQ(queue[1]->id, id2);
    EXPECT_EQ(queue[1]->message, message2_in_queue);
    EXPECT_EQ(queue[2]->id, id3);
    EXPECT_EQ(queue[2]->message, message3_in_queue);
  }

  // wrong order
  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id2);
  EXPECT_EQ(GetSignMessageQueueSize(), 3u);
  EXPECT_EQ(GetSignMessageQueueFront()->id, id1);
  EXPECT_EQ(GetSignMessageQueueFront()->message, message1_in_queue);

  brave_wallet_service_->NotifySignMessageHardwareRequestProcessed(true, id3,
                                                                   "", "");
  EXPECT_EQ(GetSignMessageQueueSize(), 3u);
  EXPECT_EQ(GetSignMessageQueueFront()->id, id1);
  EXPECT_EQ(GetSignMessageQueueFront()->message, message1_in_queue);

  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id1);
  EXPECT_EQ(GetSignMessageQueueSize(), 2u);
  EXPECT_EQ(GetSignMessageQueueFront()->id, id2);
  EXPECT_EQ(GetSignMessageQueueFront()->message, message2_in_queue);
  {
    auto queue = GetPendingSignMessageRequests();
    ASSERT_EQ(queue.size(), 2u);
    EXPECT_EQ(queue[0]->id, id2);
    EXPECT_EQ(queue[0]->message, message2_in_queue);
    EXPECT_EQ(queue[1]->id, id3);
    EXPECT_EQ(queue[1]->message, message3_in_queue);
  }

  // old id
  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id1);
  EXPECT_EQ(GetSignMessageQueueSize(), 2u);
  EXPECT_EQ(GetSignMessageQueueFront()->id, id2);
  EXPECT_EQ(GetSignMessageQueueFront()->message, message2_in_queue);

  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id2);
  EXPECT_EQ(GetSignMessageQueueSize(), 1u);
  EXPECT_EQ(GetSignMessageQueueFront()->id, id3);
  EXPECT_EQ(GetSignMessageQueueFront()->message, message3_in_queue);
  {
    auto queue = GetPendingSignMessageRequests();
    ASSERT_EQ(queue.size(), 1u);
    EXPECT_EQ(queue[0]->id, id3);
    EXPECT_EQ(queue[0]->message, message3_in_queue);
  }

  brave_wallet_service_->NotifySignMessageHardwareRequestProcessed(true, id3,
                                                                   "", "");
  EXPECT_EQ(GetSignMessageQueueSize(), 0u);
  EXPECT_EQ(GetPendingSignMessageRequests().size(), 0u);
}

TEST_F(BraveWalletProviderImplUnitTest, ChainChangedEvent) {
  EXPECT_FALSE(observer_->ChainChangedFired());
  SetNetwork(mojom::kRinkebyChainId);
  EXPECT_TRUE(observer_->ChainChangedFired());
  EXPECT_EQ(mojom::kRinkebyChainId, observer_->GetChainId());

  // Works a second time
  observer_->Reset();
  SetNetwork(mojom::kMainnetChainId);
  EXPECT_TRUE(observer_->ChainChangedFired());
  EXPECT_EQ(mojom::kMainnetChainId, observer_->GetChainId());
}

TEST_F(BraveWalletProviderImplUnitTest, AccountsChangedEvent) {
  CreateWallet();
  AddAccount();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  EXPECT_FALSE(observer_->AccountsChangedFired());
  AddEthereumPermission(GetOrigin());
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ(std::vector<std::string>{from()}, observer_->GetAccounts());
  observer_->Reset();

  // Locking the account fires an event change with no accounts
  Lock();
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ(std::vector<std::string>(), observer_->GetAccounts());
  observer_->Reset();

  // Unlocking also fires an event wit the same account list as before
  Unlock();
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ(std::vector<std::string>{from()}, observer_->GetAccounts());
  observer_->Reset();

  // Does not fire for a different origin that has no permissions
  Navigate(GURL("https://bravesoftware.com"));
  AddEthereumPermission(GetOrigin(), 1);
  SetSelectedAccount(from(0), mojom::CoinType::ETH);
  EXPECT_FALSE(observer_->AccountsChangedFired());
}

TEST_F(BraveWalletProviderImplUnitTest, Web3ClientVersion) {
  std::string expected_version = base::StringPrintf(
      "BraveWallet/v%s", version_info::GetBraveChromiumVersionNumber().c_str());
  std::string version;
  mojom::ProviderError error;
  std::string error_message;
  Web3ClientVersion(&version, &error, &error_message);
  EXPECT_EQ(version, expected_version);
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
}

TEST_F(BraveWalletProviderImplUnitTest, AccountsChangedEventSelectedAccount) {
  CreateWallet();
  AddAccount();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);

  // Multiple accounts can be returned
  AddEthereumPermission(GetOrigin(), 0);
  AddEthereumPermission(GetOrigin(), 1);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ((std::vector<std::string>{from(0), from(1)}),
            observer_->GetAccounts());
  observer_->Reset();

  // Changing the selected account only returns that account
  SetSelectedAccount(from(0), mojom::CoinType::ETH);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ((std::vector<std::string>{from(0)}), observer_->GetAccounts());
  observer_->Reset();

  // Changing to a different allowed account only returns that account
  SetSelectedAccount(from(1), mojom::CoinType::ETH);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ((std::vector<std::string>{from(1)}), observer_->GetAccounts());
  observer_->Reset();

  // Changing gto a not allowed account returns all allowed accounts
  SetSelectedAccount(from(2), mojom::CoinType::ETH);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ((std::vector<std::string>{from(0), from(1)}),
            observer_->GetAccounts());
  observer_->Reset();

  // Resetting with multiple accounts works
  ResetEthereumPermission(GetOrigin(), 1);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ((std::vector<std::string>{from(0)}), observer_->GetAccounts());
  observer_->Reset();
}

TEST_F(BraveWalletProviderImplUnitTest, GetAllowedAccounts) {
  CreateWallet();
  AddAccount();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);

  std::string account0 = from(0);
  std::string account1 = from(1);

  // When nothing is allowed, empty array should be returned
  EXPECT_EQ(GetAllowedAccounts(false), std::vector<std::string>());
  EXPECT_EQ(GetAllowedAccounts(true), std::vector<std::string>());

  // Allowing 1 account should return that account for allowed accounts
  AddEthereumPermission(GetOrigin(), 0);
  EXPECT_EQ(GetAllowedAccounts(false), std::vector<std::string>{account0});
  EXPECT_EQ(GetAllowedAccounts(true), std::vector<std::string>{account0});

  // Multiple accounts can be returned
  AddEthereumPermission(GetOrigin(), 1);
  EXPECT_EQ(GetAllowedAccounts(false),
            (std::vector<std::string>{account0, account1}));
  EXPECT_EQ(GetAllowedAccounts(true),
            (std::vector<std::string>{account0, account1}));

  // Resetting permissions should return the remaining allowed account
  ResetEthereumPermission(GetOrigin(), 1);
  EXPECT_EQ(GetAllowedAccounts(false), std::vector<std::string>{account0});
  EXPECT_EQ(GetAllowedAccounts(true), std::vector<std::string>{account0});

  // Locking the keyring does not return any accounts
  Lock();
  EXPECT_EQ(GetAllowedAccounts(false), std::vector<std::string>());
  EXPECT_EQ(GetAllowedAccounts(true), std::vector<std::string>{account0});

  // Unlocking restores the accounts that were previously allowed
  Unlock();
  EXPECT_EQ(GetAllowedAccounts(false), std::vector<std::string>{account0});
  EXPECT_EQ(GetAllowedAccounts(true), std::vector<std::string>{account0});

  // Selected account should filter the accounts returned
  AddEthereumPermission(GetOrigin(), 1);
  SetSelectedAccount(from(0), mojom::CoinType::ETH);
  EXPECT_EQ(GetAllowedAccounts(false), std::vector<std::string>{account0});
  EXPECT_EQ(GetAllowedAccounts(true), std::vector<std::string>{account0});
  SetSelectedAccount(from(1), mojom::CoinType::ETH);
  EXPECT_EQ(GetAllowedAccounts(false), std::vector<std::string>{account1});
  EXPECT_EQ(GetAllowedAccounts(true), std::vector<std::string>{account1});
  SetSelectedAccount(from(2), mojom::CoinType::ETH);
  EXPECT_EQ(GetAllowedAccounts(false),
            (std::vector<std::string>{account0, account1}));
  EXPECT_EQ(GetAllowedAccounts(true),
            (std::vector<std::string>{account0, account1}));

  // Resetting all accounts should return an empty array again
  ResetEthereumPermission(GetOrigin(), 0);
  ResetEthereumPermission(GetOrigin(), 1);
  EXPECT_EQ(GetAllowedAccounts(false), std::vector<std::string>());
  EXPECT_EQ(GetAllowedAccounts(true), std::vector<std::string>());
}

TEST_F(BraveWalletProviderImplUnitTest, SignMessageHardware) {
  CreateWallet();
  std::string address = "0xA99D71De40D67394eBe68e4D0265cA6C9D421029";
  AddHardwareAccount(address);
  std::string signature;
  std::string expected_signature = "0xExpectedSignature";
  mojom::ProviderError error;
  std::string error_message;
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin(), address);

  // success
  SignMessageHardware(true, address, "0x1234", expected_signature, "",
                      &signature, &error, &error_message);
  EXPECT_FALSE(signature.empty());
  EXPECT_EQ(signature, expected_signature);
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());

  // forwarding errors from javascript
  std::string expected_error = "error text";
  SignMessageHardware(false, address, "0x1234", expected_signature,
                      expected_error, &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kInternalError);
  EXPECT_EQ(error_message, expected_error);

  // user rejected request
  SignMessageHardware(false, address, "0x1234", expected_signature, "",
                      &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
}

TEST_F(BraveWalletProviderImplUnitTest, SwitchEthereumChain) {
  // CreateWallet();
  CreateBraveWalletTabHelper();
  Navigate(GURL("https://bravesoftware.com"));
  brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);
  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;

  // chain doesn't exist yet
  std::string chain_id = "0x111";
  SwitchEthereumChain(chain_id, absl::nullopt, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kUnknownChain);
  EXPECT_EQ(error_message,
            l10n_util::GetStringFUTF8(IDS_WALLET_UNKNOWN_CHAIN,
                                      base::ASCIIToUTF16(chain_id)));
  EXPECT_FALSE(brave_wallet_tab_helper()->IsShowingBubble());

  // already on this chain
  SwitchEthereumChain("0x1", absl::nullopt, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  EXPECT_FALSE(brave_wallet_tab_helper()->IsShowingBubble());

  // user rejected
  SwitchEthereumChain("0x4", false, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  EXPECT_TRUE(brave_wallet_tab_helper()->IsShowingBubble());
  brave_wallet_tab_helper()->CloseBubble();
  EXPECT_FALSE(brave_wallet_tab_helper()->IsShowingBubble());

  // user approved
  SwitchEthereumChain("0x4", true, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  EXPECT_TRUE(brave_wallet_tab_helper()->IsShowingBubble());
  brave_wallet_tab_helper()->CloseBubble();
  EXPECT_FALSE(brave_wallet_tab_helper()->IsShowingBubble());
  EXPECT_EQ(json_rpc_service()->GetChainId(mojom::CoinType::ETH), "0x4");

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
  SwitchEthereumChain("0x1", absl::nullopt, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
  json_rpc_service()->NotifySwitchChainRequestProcessed(true, GetOrigin());
  run_loop.Run();
  EXPECT_EQ(json_rpc_service()->GetChainId(mojom::CoinType::ETH), "0x1");
}

TEST_F(BraveWalletProviderImplUnitTest, AddEthereumChainSwitchesForInnactive) {
  CreateBraveWalletTabHelper();
  Navigate(GURL("https://bravesoftware.com"));
  brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);

  // AddEthereumChain switches for already added networks
  std::string params = R"({"params": [{
        "chainId": "0x3",
        "chainName": "Ropsten",
        "rpcUrls": ["https://ropsten-infura.brave.com/"]
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
  json_rpc_service_->NotifySwitchChainRequestProcessed(true, GetOrigin());
  run_loop.Run();
  brave_wallet_tab_helper()->CloseBubble();
  EXPECT_FALSE(brave_wallet_tab_helper()->IsShowingBubble());
  EXPECT_EQ(json_rpc_service()->GetChainId(mojom::CoinType::ETH), "0x3");
}

TEST_F(BraveWalletProviderImplUnitTest, AddSuggestToken) {
  CreateBraveWalletTabHelper();
  Navigate(GURL("https://brave.com"));
  brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);

  mojom::BlockchainTokenPtr token = mojom::BlockchainToken::New(
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "BAT", "", true, false,
      "BAT", 18, true, "", "", "0x1", mojom::CoinType::ETH);
  bool approved = false;
  mojom::ProviderError error;
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

TEST_F(BraveWalletProviderImplUnitTest, GetEncryptionPublicKey) {
  RestoreWallet(kMnemonic1, "brave", false);
  CreateBraveWalletTabHelper();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin());
  brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);

  // Happy path
  std::string key;
  mojom::ProviderError error;
  std::string error_message;
  GetEncryptionPublicKey(from(), true, &key, &error, &error_message);
  EXPECT_EQ(key, "GeiNTGIpEKEVFeMBpd3aVs/S2EjoF8FOoichRuqjBg0=");
  EXPECT_EQ(mojom::ProviderError::kSuccess, error);
  EXPECT_TRUE(error_message.empty());

  // Locked should give invalid params error
  std::string from_address = from();
  Lock();
  GetEncryptionPublicKey(from_address, true, &key, &error, &error_message);
  EXPECT_TRUE(key.empty());
  EXPECT_EQ(mojom::ProviderError::kInvalidParams, error);
  EXPECT_FALSE(error_message.empty());

  // Unlocked and user rejected
  Unlock();
  GetEncryptionPublicKey(from(), false, &key, &error, &error_message);
  EXPECT_TRUE(key.empty());
  EXPECT_EQ(mojom::ProviderError::kUserRejectedRequest, error);
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
            error_message);

  // Address without permissions gives the invalid params error
  AddAccount();
  GetEncryptionPublicKey(from(1), true, &key, &error, &error_message);
  EXPECT_TRUE(key.empty());
  EXPECT_EQ(mojom::ProviderError::kInvalidParams, error);
  EXPECT_FALSE(error_message.empty());

  // Invalid address gives the invalid params error
  GetEncryptionPublicKey("", true, &key, &error, &error_message);
  EXPECT_TRUE(key.empty());
  EXPECT_EQ(mojom::ProviderError::kInvalidParams, error);
  EXPECT_FALSE(error_message.empty());
}

TEST_F(BraveWalletProviderImplUnitTest, Decrypt) {
  RestoreWallet(kMnemonic1, "brave", false);
  CreateBraveWalletTabHelper();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(GetOrigin());
  brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);

  std::string valid_pi_json =
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"6IWDnjTObWyEB/XpQWT9Rs6CTed24BaA","ephemPublicKey":"XhoADVJjjmI5iUveoJ8sm3v9+wWBwCN6x/6K2tFhdg8=","ciphertext":"lru72L3/fK+X30ZBTxhVmp1YDTb0CZ+NAAxG919PJR9Y0icmpjhEijoASBLB2kR1KfKMtERHxpeCl9XYtmRY87LBRIuRFAmvoA6j0kF4YhDSm4AzMpwQRzvZSIC49rLHJZM1rSDLBMKkFdON0H3D"})";
  std::string empty_message_json =
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"X0HlUQmgWwjiB0794AB4Js/wbzjrM9v9","ephemPublicKey":"nf595GsfgQKpQahDibdvFsxjOCG4j8luJ+fM5WIjoGQ=","ciphertext":"jvRnfKcpv4t1Oghb+q4vqw=="})";

  // Happy path w/ key GeiNTGIpEKEVFeMBpd3aVs/S2EjoF8FOoichRuqjBg0=
  std::string unsafe_message;
  mojom::ProviderError error;
  std::string error_message;
  Decrypt(valid_pi_json, from(), true, &unsafe_message, &error, &error_message);
  EXPECT_EQ(unsafe_message,
            "3."
            "141592653589793238462643383279502884197169399375105820974944592307"
            "816406286208998628034825...");
  EXPECT_EQ(mojom::ProviderError::kSuccess, error);
  EXPECT_TRUE(error_message.empty());

  // Happy path w/ empty message
  Decrypt(empty_message_json, from(), true, &unsafe_message, &error,
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
    Decrypt(error_case, from(), true, &unsafe_message, &error, &error_message);
    EXPECT_TRUE(unsafe_message.empty()) << " case: " << error_case;
    EXPECT_EQ(mojom::ProviderError::kInvalidParams, error)
        << " case: " << error_case;
    EXPECT_FALSE(error_message.empty()) << " case: " << error_case;
  }

  // Locked should give invalid params error
  std::string from_address = from();
  Lock();
  Decrypt(valid_pi_json, from_address, true, &unsafe_message, &error,
          &error_message);
  EXPECT_TRUE(unsafe_message.empty());
  EXPECT_EQ(mojom::ProviderError::kInvalidParams, error);
  EXPECT_FALSE(error_message.empty());

  // Unlocked and user rejected
  Unlock();
  Decrypt(valid_pi_json, from(), false, &unsafe_message, &error,
          &error_message);
  EXPECT_TRUE(unsafe_message.empty());
  EXPECT_EQ(mojom::ProviderError::kUserRejectedRequest, error);
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
            error_message);

  // Address without permissions gives the invalid params error
  AddAccount();
  Decrypt(valid_pi_json, from(1), true, &unsafe_message, &error,
          &error_message);
  EXPECT_TRUE(unsafe_message.empty());
  EXPECT_EQ(mojom::ProviderError::kInvalidParams, error);
  EXPECT_FALSE(error_message.empty());

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
      from(), true, &unsafe_message, &error, &error_message);
  EXPECT_TRUE(unsafe_message.empty());
  EXPECT_EQ(mojom::ProviderError::kInvalidParams, error);
  EXPECT_FALSE(error_message.empty());
}

}  // namespace brave_wallet
