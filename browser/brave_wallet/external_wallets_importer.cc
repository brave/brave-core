/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/external_wallets_importer.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversion_utils.h"
#include "base/task/bind_post_task.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/password_encryptor.h"
#include "brave/third_party/argon2/src/include/argon2.h"
#include "components/value_store/value_store.h"
#include "extensions/browser/api/storage/backend_task_runner.h"
#include "extensions/browser/api/storage/storage_frontend.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"
#include "extensions/common/mojom/manifest.mojom.h"
#include "third_party/boringssl/src/include/openssl/digest.h"
#include "third_party/boringssl/src/include/openssl/hkdf.h"

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service_factory.h"
#endif

using extensions::Extension;
using extensions::ExtensionRegistry;
using extensions::IsOnBackendSequence;
using extensions::StorageFrontend;
using extensions::mojom::ManifestLocation;
using value_store::ValueStore;

namespace brave_wallet {

namespace {

void OnRunWithStorage(
    base::OnceCallback<void(std::unique_ptr<base::DictionaryValue>)> callback,
    ValueStore* storage) {
  DCHECK(IsOnBackendSequence());
  DCHECK(storage);
  ValueStore::ReadResult result = storage->Get();
  std::move(callback).Run(result.PassSettings());
}

static base::span<const uint8_t> ToSpan(base::StringPiece sp) {
  return base::as_bytes(base::make_span(sp));
}

std::string GetLegacyCryptoWalletsPassword(const std::string& password,
                                           base::Value&& dict) {
  std::string legacy_crypto_wallets_password;
  const base::Value* argon_params_value =
      dict.FindPath("data.KeyringController.argonParams");
  CHECK(argon_params_value);
  if (!argon_params_value->is_dict()) {
    VLOG(0) << "data.KeyringController.argonParams is not dict";
    return std::string();
  }
  auto hash_len = argon_params_value->FindIntKey("hashLen");
  auto mem = argon_params_value->FindIntKey("mem");
  auto time = argon_params_value->FindIntKey("time");
  auto type = argon_params_value->FindIntKey("type");
  if (!hash_len || !mem || !time) {
    VLOG(0) << "missing hashLen, mem, time or type in argonParams";
    return std::string();
  }

  if (type != 2) {
    VLOG(0) << "Type should be Argon2_id";
    return std::string();
  }

  const std::string* salt_str =
      dict.FindStringPath("data.KeyringController.salt");
  if (!salt_str) {
    VLOG(0) << "missing data.KeyringController.salt";
    return std::string();
  }

  // We need to count characters here because js implemenation forcibly utf8
  // decode random bytes
  // (https://github.com/brave/KeyringController/blob/0769514cea07e85ae190f30765d0a301c631c56b/index.js#L91)
  // and causes 0xEFBFBD which is � (code point 0xFFFD) to be // NOLINT
  // inserted and replace the original byte when it is not a valid unicode
  // encoding. When we pass salt to argon2, argon2 decides salt size by
  // salt.length which would be 32 because it counts character length not
  // bytes size
  // https://github.com/urbit/argon2-wasm/blob/c9e73723cebe3d76cf286f5c7709b64edb25c684/index.js#L73
  size_t character_count = 0;
  for (int32_t i = 0; i < (int32_t)salt_str->size(); ++i) {
    uint32_t code_point;
    if (base::ReadUnicodeCharacter((const char*)salt_str->data(),
                                   salt_str->size(), &i, &code_point))
      ++character_count;
  }

  std::vector<uint8_t> master_key(*hash_len);
  if (argon2id_hash_raw(*time, *mem, 1, password.data(), password.size(),
                        salt_str->data(), character_count, master_key.data(),
                        *hash_len) != ARGON2_OK) {
    VLOG(1) << "argon2id_hash_raw failed";
    return std::string();
  }
  const std::string info = "metamask-encryptor";
  std::vector<uint8_t> sub_key(*hash_len);
  if (!HKDF(sub_key.data(), sub_key.size(), EVP_sha512(), master_key.data(),
            master_key.size(), nullptr, 0, (uint8_t*)info.data(),
            info.size())) {
    VLOG(1) << "HKDF failed";
    return std::string();
  }

  // We need to go through whole buffer trying to see if there is an invalid
  // unicdoe encoding and replace it with � (code point 0xFFFD) // NOLINT
  // because js implementation forcibly utf8 decode sub_key
  // https://github.com/brave/KeyringController/blob/0769514cea07e85ae190f30765d0a301c631c56b/index.js#L547
  for (int32_t i = 0; i < (int32_t)sub_key.size(); ++i) {
    uint32_t code_point;
    if (!base::ReadUnicodeCharacter((const char*)sub_key.data(), sub_key.size(),
                                    &i, &code_point) ||
        !base::IsValidCodepoint(code_point)) {
      code_point = 0xfffd;
    }
    base::WriteUnicodeCharacter(code_point, &legacy_crypto_wallets_password);
  }

  return legacy_crypto_wallets_password;
}

}  // namespace

ExternalWalletsImporter::ExternalWalletsImporter(
    mojom::ExternalWalletType type,
    content::BrowserContext* context)
    : type_(type), context_(context), weak_ptr_factory_(this) {
  DCHECK(!storage_data_);
}
ExternalWalletsImporter::~ExternalWalletsImporter() = default;

void ExternalWalletsImporter::Initialize(InitCallback callback) {
  const Extension* extension = nullptr;
  if (type_ == mojom::ExternalWalletType::CryptoWallets) {
#if !BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
    std::move(callback).Run(false);
    return;
#endif
    extension = GetCryptoWallets();
    // Crypto Wallets is not loaded
    if (!extension) {
      EthereumRemoteClientService* service =
          EthereumRemoteClientServiceFactory::GetInstance()->GetForContext(
              context_);
      DCHECK(service);
      service->MaybeLoadCryptoWalletsExtension(
          base::BindOnce(&ExternalWalletsImporter::OnCryptoWalletsLoaded,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
      return;
    }
  } else if (type_ == mojom::ExternalWalletType::MetaMask) {
    extension = GetMetaMask();
    if (!extension) {
      VLOG(1) << "Failed to load MetaMask extension";
      std::move(callback).Run(false);
      return;
    }
  }

  GetLocalStorage(extension, std::move(callback));
}

bool ExternalWalletsImporter::IsInitialized() const {
  return !!storage_data_;
}

void ExternalWalletsImporter::OnCryptoWalletsLoaded(InitCallback callback) {
  const Extension* extension = GetCryptoWallets();
  if (!extension) {
    VLOG(1) << "Failed to load Crypto Wallets extension";
    std::move(callback).Run(false);
    return;
  }

  GetLocalStorage(extension, std::move(callback));

  EthereumRemoteClientService* service =
      EthereumRemoteClientServiceFactory::GetInstance()->GetForContext(
          context_);
  DCHECK(service);
  service->UnloadCryptoWalletsExtension();
}

bool ExternalWalletsImporter::IsCryptoWalletsInstalledInternal() const {
  if (!extensions::ExtensionPrefs::Get(context_)->HasPrefForExtension(
          ethereum_remote_client_extension_id))
    return false;
  return true;
}

bool ExternalWalletsImporter::IsExternalWalletInstalled() const {
  if (is_external_wallet_installed_for_testing_)
    return true;
  if (type_ == mojom::ExternalWalletType::CryptoWallets) {
#if !BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
    return false;
#endif
    if (!IsCryptoWalletsInstalledInternal())
      return false;
  } else if (type_ == mojom::ExternalWalletType::MetaMask) {
    if (!GetMetaMask())
      return false;
  }
  return true;
}

bool ExternalWalletsImporter::IsExternalWalletInitialized() const {
  if (!IsInitialized())
    return false;
  return storage_data_->FindPath("data.KeyringController");
}

void ExternalWalletsImporter::GetImportInfo(
    const std::string& password,
    GetImportInfoCallback callback) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!IsInitialized()) {
    std::move(callback).Run(false, ImportInfo(), ImportError::kInternalError);
    return;
  }

  if (password.empty()) {
    VLOG(1) << "password is empty";
    std::move(callback).Run(false, ImportInfo(), ImportError::kPasswordError);
    return;
  }

  if (storage_data_->FindPath("data.KeyringController.argonParams")) {
    auto storage_data_clone = storage_data_->Clone();
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&GetLegacyCryptoWalletsPassword, password,
                       std::move(storage_data_clone)),
        base::BindOnce(&ExternalWalletsImporter::GetMnemonic,
                       weak_ptr_factory_.GetWeakPtr(), true,
                       std::move(callback)));
  } else {
    GetMnemonic(false, std::move(callback), password);
  }
}

const Extension* ExternalWalletsImporter::GetCryptoWallets() const {
  ExtensionRegistry* registry = ExtensionRegistry::Get(context_);
  if (!registry)
    return nullptr;
  return registry->GetInstalledExtension(ethereum_remote_client_extension_id);
}

const Extension* ExternalWalletsImporter::GetMetaMask() const {
  ExtensionRegistry* registry = ExtensionRegistry::Get(context_);
  if (!registry)
    return nullptr;
  return registry->GetInstalledExtension(metamask_extension_id);
}

void ExternalWalletsImporter::GetLocalStorage(
    const extensions::Extension* extension,
    InitCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string error;
  extension_ = Extension::Create(
      extension->path(), ManifestLocation::kExternalPref,
      *extension->manifest()->value(), extension->creation_flags(), &error);

  StorageFrontend* frontend = StorageFrontend::Get(context_);
  if (!frontend) {
    VLOG(1) << "Failed to read chrome.storage.local";
    std::move(callback).Run(false);
    return;
  }

  // Passing the result back using BindPostTask because OnRunWithStorage will
  // run on backend thread
  frontend->RunWithStorage(
      extension_, extensions::settings_namespace::LOCAL,
      base::BindOnce(
          &OnRunWithStorage,
          base::BindPostTask(
              base::SequencedTaskRunnerHandle::Get(),
              base::BindOnce(&ExternalWalletsImporter::OnGetLocalStorage,
                             weak_ptr_factory_.GetWeakPtr(),
                             std::move(callback)))));
}

void ExternalWalletsImporter::OnGetLocalStorage(
    InitCallback callback,
    std::unique_ptr<base::DictionaryValue> dict) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  storage_data_ = std::move(dict);
  std::move(callback).Run(true);
}

void ExternalWalletsImporter::GetMnemonic(bool is_legacy_crypto_wallets,
                                          GetImportInfoCallback callback,
                                          const std::string& password) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());

  if (password.empty()) {
    VLOG(0) << "Failed to get password of legacy Crypto Wallets";
    std::move(callback).Run(false, ImportInfo(), ImportError::kInternalError);
    return;
  }

  const std::string* vault_str =
      storage_data_->FindStringPath("data.KeyringController.vault");
  if (!vault_str) {
    VLOG(0) << "cannot find data.KeyringController.vault";
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }
  auto vault =
      base::JSONReader::Read(*vault_str, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                             base::JSON_ALLOW_TRAILING_COMMAS);
  if (!vault) {
    VLOG(1) << "not a valid JSON: " << *vault_str;
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }
  auto* data_str = vault->FindStringKey("data");
  auto* iv_str = vault->FindStringKey("iv");
  auto* salt_str = vault->FindStringKey("salt");
  if (!data_str || !iv_str || !salt_str) {
    VLOG(1) << "data or iv or salt is missing";
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }

  std::string salt_decoded;
  if (!base::Base64Decode(*salt_str, &salt_decoded)) {
    VLOG(1) << "base64 decode failed: " << *salt_str;
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }
  std::string iv_decoded;
  if (!base::Base64Decode(*iv_str, &iv_decoded)) {
    VLOG(1) << "base64 decode failed: " << *iv_str;
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }
  std::string data_decoded;
  if (!base::Base64Decode(*data_str, &data_decoded)) {
    VLOG(1) << "base64 decode failed: " << *data_str;
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }

  std::unique_ptr<PasswordEncryptor> encryptor =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          password, ToSpan(salt_decoded), 10000, 256);
  DCHECK(encryptor);

  std::vector<uint8_t> decrypted_keyrings;
  if (!encryptor->DecryptForImporter(ToSpan(data_decoded), ToSpan(iv_decoded),
                                     &decrypted_keyrings)) {
    VLOG(0) << "Importer decryption failed";
    std::move(callback).Run(false, ImportInfo(), ImportError::kPasswordError);
    return;
  }

  const std::string decrypted_keyrings_str =
      std::string(decrypted_keyrings.begin(), decrypted_keyrings.end());
  auto keyrings = base::JSONReader::Read(
      decrypted_keyrings_str,
      base::JSON_PARSE_CHROMIUM_EXTENSIONS | base::JSON_ALLOW_TRAILING_COMMAS);
  if (!keyrings) {
    VLOG(1) << "not a valid JSON: " << decrypted_keyrings_str;
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }

  const std::string* mnemonic = nullptr;
  absl::optional<int> number_of_accounts = absl::nullopt;
  for (const auto& keyring : keyrings->GetListDeprecated()) {
    DCHECK(keyring.is_dict());
    const auto* type = keyring.FindStringKey("type");
    if (!type) {
      VLOG(0) << "keyring.type is missing";
      std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
      return;
    }
    if (*type != "HD Key Tree")
      continue;
    mnemonic = keyring.FindStringPath("data.mnemonic");
    if (!mnemonic) {
      VLOG(0) << "keyring.data.menmonic is missing";
      std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
      return;
    }
    number_of_accounts = keyring.FindIntPath("data.numberOfAccounts");
    break;
  }

  if (!mnemonic) {
    VLOG(0) << "Failed to find mnemonic in decrypted keyrings";
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
  }

  std::move(callback).Run(
      true,
      ImportInfo(
          {*mnemonic, is_legacy_crypto_wallets,
           number_of_accounts ? static_cast<size_t>(*number_of_accounts) : 1}),
      ImportError::kNone);
}

void ExternalWalletsImporter::SetStorageDataForTesting(
    std::unique_ptr<base::DictionaryValue> data) {
  storage_data_ = std::move(data);
}

void ExternalWalletsImporter::SetExternalWalletInstalledForTesting(
    bool installed) {
  is_external_wallet_installed_for_testing_ = installed;
}

}  // namespace brave_wallet
