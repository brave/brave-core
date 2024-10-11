/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/external_wallets_importer.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversion_utils.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/types/cxx23_to_underlying.h"
#include "base/types/fixed_array.h"
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

void OnRunWithStorage(base::OnceCallback<void(base::Value::Dict)> callback,
                      ValueStore* storage) {
  DCHECK(IsOnBackendSequence());
  DCHECK(storage);
  std::move(callback).Run(storage->Get().PassSettings());
}

std::string GetLegacyCryptoWalletsPassword(const std::string& password,
                                           base::Value::Dict dict) {
  std::string legacy_crypto_wallets_password;
  const auto* argon_params =
      dict.FindDictByDottedPath("data.KeyringController.argonParams");
  if (!argon_params) {
    VLOG(0) << "data.KeyringController.argonParams is not dict";
    return std::string();
  }
  auto hash_len = argon_params->FindInt("hashLen");
  auto mem = argon_params->FindInt("mem");
  auto time = argon_params->FindInt("time");
  auto type = argon_params->FindInt("type");
  if (!hash_len || !mem || !time) {
    VLOG(0) << "missing hashLen, mem, time or type in argonParams";
    return std::string();
  }

  if (type != 2) {
    VLOG(0) << "Type should be Argon2_id";
    return std::string();
  }

  const std::string* salt_str =
      dict.FindStringByDottedPath("data.KeyringController.salt");
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
  for (size_t i = 0; i < salt_str->size(); ++i) {
    base_icu::UChar32 code_point;
    if (base::ReadUnicodeCharacter(salt_str->data(), salt_str->size(), &i,
                                   &code_point)) {
      ++character_count;
    }
  }

  base::FixedArray<uint8_t> master_key(*hash_len);
  if (argon2id_hash_raw(*time, *mem, 1, password.data(), password.size(),
                        salt_str->data(), character_count, master_key.data(),
                        *hash_len) != ARGON2_OK) {
    VLOG(1) << "argon2id_hash_raw failed";
    return std::string();
  }
  const std::string info = "metamask-encryptor";
  base::FixedArray<uint8_t> sub_key(*hash_len);
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
  for (size_t i = 0; i < sub_key.size(); ++i) {
    base_icu::UChar32 code_point;
    if (!base::ReadUnicodeCharacter(
            reinterpret_cast<const char*>(sub_key.data()), sub_key.size(), &i,
            &code_point) ||
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
  DCHECK(!storage_data_.has_value());
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
  } else {
    NOTREACHED_IN_MIGRATION() << "Unsupported ExternalWalletType type. value="
                              << base::to_underlying(type_);
    std::move(callback).Run(false);
    return;
  }

  GetLocalStorage(*extension, std::move(callback));
}

bool ExternalWalletsImporter::IsInitialized() const {
  return storage_data_.has_value();
}

void ExternalWalletsImporter::OnCryptoWalletsLoaded(InitCallback callback) {
  const Extension* extension = GetCryptoWallets();
  if (!extension) {
    VLOG(1) << "Failed to load Crypto Wallets extension";
    std::move(callback).Run(false);
    return;
  }

  GetLocalStorage(*extension, std::move(callback));

  EthereumRemoteClientService* service =
      EthereumRemoteClientServiceFactory::GetInstance()->GetForContext(
          context_);
  DCHECK(service);
  service->UnloadCryptoWalletsExtension();
}

bool ExternalWalletsImporter::IsCryptoWalletsInstalledInternal() const {
  if (!extensions::ExtensionPrefs::Get(context_)->HasPrefForExtension(
          kEthereumRemoteClientExtensionId)) {
    return false;
  }
  return true;
}

bool ExternalWalletsImporter::IsExternalWalletInstalled() const {
  if (is_external_wallet_installed_for_testing_) {
    return true;
  }
  if (type_ == mojom::ExternalWalletType::CryptoWallets) {
#if !BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
    return false;
#endif
    if (!IsCryptoWalletsInstalledInternal()) {
      return false;
    }
  } else if (type_ == mojom::ExternalWalletType::MetaMask) {
    if (!GetMetaMask()) {
      return false;
    }
  }
  return true;
}

bool ExternalWalletsImporter::IsExternalWalletInitialized() const {
  if (!IsInitialized()) {
    return false;
  }
  return storage_data_->FindByDottedPath("data.KeyringController") != nullptr;
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

  if (storage_data_->FindByDottedPath("data.KeyringController.argonParams")) {
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&GetLegacyCryptoWalletsPassword, password,
                       storage_data_->Clone()),
        base::BindOnce(&ExternalWalletsImporter::GetMnemonic,
                       weak_ptr_factory_.GetWeakPtr(), true,
                       std::move(callback)));
  } else {
    GetMnemonic(false, std::move(callback), password);
  }
}

const Extension* ExternalWalletsImporter::GetCryptoWallets() const {
  ExtensionRegistry* registry = ExtensionRegistry::Get(context_);
  if (!registry) {
    return nullptr;
  }
  return registry->GetInstalledExtension(kEthereumRemoteClientExtensionId);
}

const Extension* ExternalWalletsImporter::GetMetaMask() const {
  ExtensionRegistry* registry = ExtensionRegistry::Get(context_);
  if (!registry) {
    return nullptr;
  }
  return registry->GetInstalledExtension(kMetamaskExtensionId);
}

void ExternalWalletsImporter::GetLocalStorage(
    const extensions::Extension& extension,
    InitCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string error;
  extension_ = Extension::Create(
      extension.path(), ManifestLocation::kExternalPref,
      *extension.manifest()->value(), extension.creation_flags(), &error);

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
              base::SequencedTaskRunner::GetCurrentDefault(),
              base::BindOnce(&ExternalWalletsImporter::OnGetLocalStorage,
                             weak_ptr_factory_.GetWeakPtr(),
                             std::move(callback)))));
}

void ExternalWalletsImporter::OnGetLocalStorage(InitCallback callback,
                                                base::Value::Dict dict) {
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
      storage_data_->FindStringByDottedPath("data.KeyringController.vault");
  if (!vault_str) {
    VLOG(0) << "cannot find data.KeyringController.vault";
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }
  auto parsed_vault =
      base::JSONReader::Read(*vault_str, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                             base::JSON_ALLOW_TRAILING_COMMAS);
  auto* vault = parsed_vault ? parsed_vault->GetIfDict() : nullptr;
  if (!vault) {
    VLOG(1) << "not a valid JSON: " << *vault_str;
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }
  auto* data_str = vault->FindString("data");
  auto* iv_str = vault->FindString("iv");
  auto* salt_str = vault->FindString("salt");
  if (!data_str || !iv_str || !salt_str) {
    VLOG(1) << "data or iv or salt is missing";
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }

  auto salt_decoded = base::Base64Decode(*salt_str);
  if (!salt_decoded) {
    VLOG(1) << "base64 decode failed: " << *salt_str;
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }
  auto iv_decoded = base::Base64Decode(*iv_str);
  if (!iv_decoded) {
    VLOG(1) << "base64 decode failed: " << *iv_str;
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }
  auto data_decoded = base::Base64Decode(*data_str);
  if (!data_decoded) {
    VLOG(1) << "base64 decode failed: " << *data_str;
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }

  std::unique_ptr<PasswordEncryptor> encryptor =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          password, *salt_decoded, 600000, 256);
  DCHECK(encryptor);

  auto decrypted_keyrings =
      encryptor->DecryptForImporter(*data_decoded, *iv_decoded);
  if (!decrypted_keyrings) {
    // Also try with legacy 10K iterations.
    std::unique_ptr<PasswordEncryptor> encryptor_10k =
        PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
            password, *salt_decoded, 10000, 256);
    DCHECK(encryptor_10k);

    decrypted_keyrings =
        encryptor_10k->DecryptForImporter(*data_decoded, *iv_decoded);
  }

  if (!decrypted_keyrings) {
    VLOG(0) << "Importer decryption failed";
    std::move(callback).Run(false, ImportInfo(), ImportError::kPasswordError);
    return;
  }

  const std::string decrypted_keyrings_str =
      std::string(decrypted_keyrings->begin(), decrypted_keyrings->end());
  auto keyrings = base::JSONReader::Read(
      decrypted_keyrings_str,
      base::JSON_PARSE_CHROMIUM_EXTENSIONS | base::JSON_ALLOW_TRAILING_COMMAS);
  if (!keyrings) {
    VLOG(1) << "not a valid JSON: " << decrypted_keyrings_str;
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }

  std::optional<std::string> mnemonic = std::nullopt;
  std::optional<int> number_of_accounts = std::nullopt;
  for (const auto& keyring_listed : keyrings->GetList()) {
    DCHECK(keyring_listed.is_dict());
    const auto& keyring = *keyring_listed.GetIfDict();
    const auto* type = keyring.FindString("type");
    if (!type) {
      VLOG(0) << "keyring.type is missing";
      std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
      return;
    }
    if (*type != "HD Key Tree") {
      continue;
    }
    const std::string* str_mnemonic =
        keyring.FindStringByDottedPath("data.mnemonic");
    // data.mnemonic is not string, try utf8 encoded byte array
    if (!str_mnemonic) {
      const auto* list = keyring.FindListByDottedPath("data.mnemonic");
      std::vector<uint8_t> utf8_encoded_mnemonic;
      if (list) {
        for (const auto& item : *list) {
          if (!item.is_int()) {
            break;
          }
          utf8_encoded_mnemonic.push_back(item.GetInt());
        }
      }
      if (utf8_encoded_mnemonic.empty()) {
        VLOG(0) << "keyring.data.menmonic is missing";

        std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
        return;
      }
      mnemonic = std::string(utf8_encoded_mnemonic.begin(),
                             utf8_encoded_mnemonic.end());
    } else {
      mnemonic = *str_mnemonic;
    }
    number_of_accounts = keyring.FindIntByDottedPath("data.numberOfAccounts");
    break;
  }

  if (!mnemonic) {
    VLOG(0) << "Failed to find mnemonic in decrypted keyrings";
    std::move(callback).Run(false, ImportInfo(), ImportError::kJsonError);
    return;
  }

  std::move(callback).Run(
      true,
      ImportInfo(
          {*mnemonic, is_legacy_crypto_wallets,
           number_of_accounts ? static_cast<size_t>(*number_of_accounts) : 1}),
      ImportError::kNone);
}

void ExternalWalletsImporter::SetStorageDataForTesting(base::Value::Dict data) {
  storage_data_ = std::move(data);
}

void ExternalWalletsImporter::SetExternalWalletInstalledForTesting(
    bool installed) {
  is_external_wallet_installed_for_testing_ = installed;
}

}  // namespace brave_wallet
