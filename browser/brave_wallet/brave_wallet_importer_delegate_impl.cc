/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_importer_delegate_impl.h"

#include <utility>

#include "base/base64.h"
#include "base/bind_post_task.h"
#include "base/json/json_reader.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service_factory.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/browser/password_encryptor.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/storage/backend_task_runner.h"
#include "extensions/browser/api/storage/storage_frontend.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/value_store/value_store.h"
#include "extensions/common/extension.h"
#include "extensions/common/mojom/manifest.mojom.h"

using content::BrowserThread;
using extensions::Extension;
using extensions::ExtensionRegistry;
using extensions::IsOnBackendSequence;
using extensions::StorageFrontend;
using extensions::mojom::ManifestLocation;

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

}  // namespace

BraveWalletImporterDelegateImpl::BraveWalletImporterDelegateImpl(
    content::BrowserContext* context)
    : context_(context), weak_ptr_factory_(this) {}

BraveWalletImporterDelegateImpl::~BraveWalletImporterDelegateImpl() = default;

void BraveWalletImporterDelegateImpl::IsBraveCryptoWalletInstalled(
    IsBraveCryptoWalletInstalledCallback callback) {
  if (!IsBraveCryptoWalletInstalledInternal())
    std::move(callback).Run(false);
  else
    std::move(callback).Run(true);
}

void BraveWalletImporterDelegateImpl::IsMetamaskInstalled(
    IsMetamaskInstalledCallback callback) {
  if (!GetMetamask())
    std::move(callback).Run(false);
  else
    std::move(callback).Run(true);
}

void BraveWalletImporterDelegateImpl::ImportFromBraveCryptoWallet(
    const std::string& password,
    const std::string& new_password,
    ImportFromBraveCryptoWalletCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context_);

  if (password.empty() || new_password.empty() ||
      !IsBraveCryptoWalletInstalledInternal()) {
    std::move(callback).Run(false);
    return;
  }

  const Extension* extension = GetBraveCryptoWallet();
  // crypto wallet is not loaded
  if (!extension) {
    EthereumRemoteClientService* service =
        EthereumRemoteClientServiceFactory::GetInstance()->GetForContext(
            context_);
    DCHECK(service);
    service->MaybeLoadCryptoWalletsExtension(base::BindOnce(
        &BraveWalletImporterDelegateImpl::OnCryptoWalletLoaded,
        weak_ptr_factory_.GetWeakPtr(), std::move(callback), true));
  } else {
    OnCryptoWalletLoaded(std::move(callback), false);
  }

  password_ = password;
  new_password_ = new_password;
}

void BraveWalletImporterDelegateImpl::ImportFromMetamask(
    const std::string& password,
    const std::string& new_password,
    ImportFromMetamaskCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context_);

  if (password.empty() || new_password.empty()) {
    std::move(callback).Run(false);
    return;
  }

  const Extension* extension = GetMetamask();
  if (!extension) {
    std::move(callback).Run(false);
    return;
  }

  GetLocalStorage(extension, std::move(callback));
  password_ = password;
  new_password_ = new_password;
}

void BraveWalletImporterDelegateImpl::OnCryptoWalletLoaded(
    ImportFromMetamaskCallback callback,
    bool should_unload) {
  const Extension* extension = GetBraveCryptoWallet();
  if (!extension) {
    std::move(callback).Run(false);
    return;
  }

  GetLocalStorage(extension, std::move(callback));

  if (should_unload) {
    EthereumRemoteClientService* service =
        EthereumRemoteClientServiceFactory::GetInstance()->GetForContext(
            context_);
    DCHECK(service);
    service->RemoveCryptoWalletExtension();
  }
}

void BraveWalletImporterDelegateImpl::GetLocalStorage(
    const extensions::Extension* extension,
    ImportFromBraveCryptoWalletCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::string error;
  extension_ = Extension::Create(
      extension->path(), ManifestLocation::kExternalPref,
      *extension->manifest()->value(), extension->creation_flags(), &error);

  StorageFrontend* frontend = StorageFrontend::Get(context_);
  if (!frontend) {
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
              base::BindOnce(
                  &BraveWalletImporterDelegateImpl ::OnGetLocalStorage,
                  weak_ptr_factory_.GetWeakPtr(), std::move(callback)))));
}

void BraveWalletImporterDelegateImpl::OnGetLocalStorage(
    ImportFromBraveCryptoWalletCallback callback,
    std::unique_ptr<base::DictionaryValue> dict) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  EnsureConnected();
  DCHECK(!password_.empty() && !new_password_.empty());

  const std::string* argon_params_str =
      dict->FindStringPath("data.KeyringController.argonParams");
  // TODO(darkdh): Introduce argon2 deps so we can decrypt 24 words of legacy
  // encrypted mnemonic
  if (argon_params_str) {
    VLOG(1) << "legacy brave crypto wallet is not supported";
    std::move(callback).Run(false);
    return;
  }

  const std::string* vault_str =
      dict->FindStringPath("data.KeyringController.vault");
  if (!vault_str) {
    VLOG(0) << "cannot find data.KeyringController.vault";
    std::move(callback).Run(false);
    return;
  }
  auto vault = base::JSONReader::Read(*vault_str);
  if (!vault) {
    VLOG(1) << "not a valid json: " << *vault_str;
    std::move(callback).Run(false);
    return;
  }
  auto* data_str = vault->FindStringKey("data");
  auto* iv_str = vault->FindStringKey("iv");
  auto* salt_str = vault->FindStringKey("salt");
  if (!data_str || !iv_str || !salt_str) {
    std::move(callback).Run(false);
    return;
  }

  std::string salt_decoded;
  if (!base::Base64Decode(*salt_str, &salt_decoded)) {
    std::move(callback).Run(false);
    return;
  }
  std::string iv_decoded;
  if (!base::Base64Decode(*iv_str, &iv_decoded)) {
    std::move(callback).Run(false);
    return;
  }
  std::string data_decoded;
  if (!base::Base64Decode(*data_str, &data_decoded)) {
    std::move(callback).Run(false);
    return;
  }

  std::unique_ptr<PasswordEncryptor> encryptor =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          std::move(password_), ToSpan(salt_decoded), 10000, 256);
  DCHECK(encryptor);

  std::vector<uint8_t> decrypted_keyrings;
  if (!encryptor->DecryptForImporter(ToSpan(data_decoded), ToSpan(iv_decoded),
                                     &decrypted_keyrings)) {
    VLOG(1) << "Importer decryption failed";
    std::move(callback).Run(false);
    return;
  }

  const std::string decrypted_keyrings_str =
      std::string(decrypted_keyrings.begin(), decrypted_keyrings.end());
  auto keyrings = base::JSONReader::Read(decrypted_keyrings_str);
  if (!keyrings) {
    VLOG(1) << "not a valid json: " << decrypted_keyrings_str;
    std::move(callback).Run(false);
    return;
  }

  const std::string* mnemonic = nullptr;
  for (const auto& keyring : keyrings->GetList()) {
    DCHECK(keyring.is_dict());
    const auto* type = keyring.FindStringKey("type");
    if (!type) {
      VLOG(0) << "keyring.type is missing";
      std::move(callback).Run(false);
      return;
    }
    if (*type != "HD Key Tree")
      continue;
    mnemonic = keyring.FindStringPath("data.mnemonic");
    if (!mnemonic) {
      VLOG(0) << "keyring.data.menmonic is missing";
      std::move(callback).Run(false);
      return;
    }
    break;
  }

  keyring_controller_->RestoreWallet(
      *mnemonic, std::move(new_password_), false,
      base::BindOnce(
          [](ImportFromBraveCryptoWalletCallback callback,
             bool is_valid_mnemonic) {
            std::move(callback).Run(is_valid_mnemonic);
          },
          std::move(callback)));
}

bool BraveWalletImporterDelegateImpl::IsLegacyBraveCryptoWallet() const {
  EthereumRemoteClientService* service =
      EthereumRemoteClientServiceFactory::GetInstance()->GetForContext(
          context_);
  DCHECK(service);
  // service->IsCryptoWalletsSetup is only for legacy 24 words mnemonic wallet
  return service->IsCryptoWalletsSetup();
}

bool BraveWalletImporterDelegateImpl::IsBraveCryptoWalletInstalledInternal()
    const {
  if (!extensions::ExtensionPrefs::Get(context_)->HasPrefForExtension(
          ethereum_remote_client_extension_id))
    return false;
  // TODO(darkdh): block legacy wallet until we support decrypting its mnemonic
  if (IsLegacyBraveCryptoWallet())
    return false;
  return true;
}

const Extension* BraveWalletImporterDelegateImpl::GetBraveCryptoWallet() {
  ExtensionRegistry* registry = ExtensionRegistry::Get(context_);
  if (!registry)
    return nullptr;
  return registry->GetInstalledExtension(ethereum_remote_client_extension_id);
}

const Extension* BraveWalletImporterDelegateImpl::GetMetamask() {
  ExtensionRegistry* registry = ExtensionRegistry::Get(context_);
  if (!registry)
    return nullptr;
  return registry->GetInstalledExtension(metamask_extension_id);
}

void BraveWalletImporterDelegateImpl::EnsureConnected() {
  if (!keyring_controller_) {
    auto pending =
        brave_wallet::KeyringControllerFactory::GetInstance()->GetForContext(
            context_);
    keyring_controller_.Bind(std::move(pending));
  }
  DCHECK(keyring_controller_);
  keyring_controller_.set_disconnect_handler(
      base::BindOnce(&BraveWalletImporterDelegateImpl::OnConnectionError,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletImporterDelegateImpl::OnConnectionError() {
  keyring_controller_.reset();
  EnsureConnected();
}

}  // namespace brave_wallet
