/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_service.h"

#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/value_iterators.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_keyring.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/ethereum_keyring.h"
#include "brave/components/brave_wallet/browser/filecoin_keyring.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/random.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/l10n/l10n_util.h"

/* kBraveWalletKeyrings structure
 *
 * "filecoin":
 *   {
 *     "selected_account": "t1....ac",
 *     "imported_accounts": [
 *       {
 *           "account_address": "t3vmv....ughsa",
 *           "account_name": "Filecoin",
 *           "encrypted_private_key": "9/Xb...X4IL",
 *           "coin_type": 461 // Enum mojom::CoinType
 *       }
 *     ],
 *     "hardware":  {
 *        ...
 *     }
 *     "password_encryptor_nonce": "xxx"
 * },
 * "default":
 *   {
 *      "selected_account": "0xb3652763...cf3744911",
 *      "backup_complete": false,
 *      "encrypted_mnemonic": [mnemonic],
 *      "legacy_brave_wallet": false,
 *      "account_metas": {
 *         "m/44'/60'/0'/0/0": {
 *               "account_name": "account 1",
 *               ...
 *               "coin_type": 60 // Enum mojom::CoinType
 *          },
 *          "m/44'/60'/0'/0/1": {
 *               "account_name": "account 2",
 *               ...
 *          }
 *      },
 *     "hardware":  {
 *       "Ledger12445": {
 *         "account_metas": {
 *           "0xEA04...CC8Acc": {
 *             "account_name": "Ledger",
 *             "derivation_path": "m/44'/60'/1'/0/0",
 *             "hardware_vendor": "ledger",
 *             "coin_type": 60 // Enum mojom::CoinType
 *           },
 *           "0x264Ef...6b8F1": {
 *             "account_name": "Ledger",
 *             "derivation_path": "m/44'/60'/2'/0/0",
 *             "hardware_vendor": "ledger",
 *             "coin_type": 60 // Enum mojom::CoinType
 *            }
 *         },
 *         device_name: "Ledger 123"
 *       },
 *       "Ledger44332":{
 *         ...
 *       }
 *     },
 *      "imported_accounts": [
 *        { "address": "0x71f430f5f2a79274c17986ea1a1106596a39ba05",
 *          "encrypted_private_key": [privatekey],
 *          "account_name": "Imported account 1"
 *        },
 *        ...
 *      ],
 *      ...
 *   },
 *
 *   [keyringid]: {...}
 *   ...
 * }
 */

namespace brave_wallet {
namespace {
const size_t kSaltSize = 32;
const size_t kNonceSize = 12;
const int kPbkdf2IterationsLegacy = 100000;
const int kPbkdf2Iterations = 310000;
const int kPbkdf2KeySize = 256;
const char kPasswordEncryptorSalt[] = "password_encryptor_salt";
const char kPasswordEncryptorNonce[] = "password_encryptor_nonce";
const char kEncryptedMnemonic[] = "encrypted_mnemonic";
const char kBackupComplete[] = "backup_complete";
const char kAccountMetas[] = "account_metas";
const char kAccountName[] = "account_name";
const char kAccountIndex[] = "account_index";
const char kHardwareVendor[] = "hardware_vendor";
const char kImportedAccounts[] = "imported_accounts";
const char kAccountAddress[] = "account_address";
const char kEncryptedPrivateKey[] = "encrypted_private_key";
const char kCoinType[] = "coin_type";
const char kLegacyBraveWallet[] = "legacy_brave_wallet";
const char kHardwareAccounts[] = "hardware";
const char kHardwareDerivationPath[] = "derivation_path";

std::string KeyringIdPrefString(mojom::KeyringId keyring_id) {
  switch (keyring_id) {
    case mojom::KeyringId::kFilecoin:
      return "filecoin";
    case mojom::KeyringId::kFilecoinTestnet:
      return "filecoin_testnet";
    case mojom::KeyringId::kSolana:
      return "solana";
    case mojom::KeyringId::kDefault:
      return "default";
    case mojom::KeyringId::kBitcoin84:
      return "bitcoin_84";
    case mojom::KeyringId::kBitcoin84Testnet:
      return "bitcoin_84_test";
  }
  NOTREACHED();
  return "";
}

std::string GetRootPath(mojom::KeyringId keyring_id) {
  if (keyring_id == mojom::kDefaultKeyringId) {
    return "m/44'/60'/0'/0";
  } else if (keyring_id == mojom::kSolanaKeyringId) {
    return "m/44'/501'";
  } else if (keyring_id == mojom::kFilecoinKeyringId) {
    return "m/44'/461'/0'/0";
  } else if (keyring_id == mojom::kFilecoinTestnetKeyringId) {
    return "m/44'/1'/0'/0";
  } else if (keyring_id == mojom::KeyringId::kBitcoin84) {
    return "m/84'/0'";
  } else if (keyring_id == mojom::KeyringId::kBitcoin84Testnet) {
    return "m/84'/1'";
  }

  NOTREACHED();
  return "";
}

absl::optional<uint32_t> ExtractAccountIndex(mojom::KeyringId keyring_id,
                                             const std::string& path) {
  CHECK(keyring_id == mojom::KeyringId::kDefault ||
        keyring_id == mojom::KeyringId::kFilecoin ||
        keyring_id == mojom::KeyringId::kFilecoinTestnet ||
        keyring_id == mojom::KeyringId::kSolana);

  // m/44'/60'/0'/0/{index}
  // m/44'/461'/0'/0/{index}
  // m/44'/1'/0'/0/{index}
  // m/44'/501'/{index}'/0'

  // For all types remove root path and slash. For Solana also remove '/0'.

  auto account_index = base::StringPiece(path);
  auto root_path = GetRootPath(keyring_id);
  if (!base::StartsWith(account_index, root_path)) {
    return absl::nullopt;
  }
  account_index.remove_prefix(root_path.size());

  if (!base::StartsWith(account_index, "/")) {
    return absl::nullopt;
  }
  account_index.remove_prefix(1);

  if (keyring_id == mojom::KeyringId::kSolana) {
    if (!base::EndsWith(account_index, "'/0'")) {
      return absl::nullopt;
    }
    account_index.remove_suffix(4);
  }

  uint32_t result = 0;
  if (!base::StringToUint(account_index, &result)) {
    return absl::nullopt;
  }

  return result;
}

static base::span<const uint8_t> ToSpan(base::StringPiece sp) {
  return base::as_bytes(base::make_span(sp));
}

std::string GetAccountName(size_t number) {
  return l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_NUMBERED_ACCOUNT_NAME,
                                   base::NumberToString16(number));
}

mojom::AccountInfoPtr MakeAccountInfoForHardwareAccount(
    const std::string& address,
    const std::string& name,
    const std::string& derivation_path,
    const std::string& hardware_vendor,
    const std::string& device_id,
    mojom::KeyringId keyring_id) {
  return mojom::AccountInfo::New(
      MakeAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                    mojom::AccountKind::kHardware, address),
      address, name,
      mojom::HardwareInfo::New(derivation_path, hardware_vendor, device_id));
}

void SerializeHardwareAccounts(const std::string& device_id,
                               const base::Value* account_value,
                               mojom::KeyringId keyring_id,
                               std::vector<mojom::AccountInfoPtr>* accounts) {
  for (const auto account : account_value->GetDict()) {
    DCHECK(account.second.is_dict());
    std::string address = account.first;
    const base::Value::Dict& dict = account.second.GetDict();

    std::string hardware_vendor;
    const std::string* hardware_value = dict.FindString(kHardwareVendor);
    if (hardware_value) {
      hardware_vendor = *hardware_value;
    }

    std::string name;
    const std::string* name_value = dict.FindString(kAccountName);
    if (name_value) {
      name = *name_value;
    }

    std::string derivation_path;
    const std::string* derivation_path_value =
        dict.FindString(kHardwareDerivationPath);
    if (derivation_path_value) {
      derivation_path = *derivation_path_value;
    }

    mojom::CoinType coin = mojom::CoinType::ETH;
    auto coin_name_value = dict.FindInt(kCoinType);
    if (coin_name_value) {
      coin = static_cast<mojom::CoinType>(*coin_name_value);
    }
    DCHECK_EQ(coin, GetCoinForKeyring(keyring_id));

    accounts->push_back(MakeAccountInfoForHardwareAccount(
        address, name, derivation_path, hardware_vendor, device_id,
        keyring_id));
  }
}

// TODO(apaymyshev): Need to use much lesser value for unit tests where this
// value is irrelevenat. Otherwise it takes too much time for tests to pass (44
// seconds for *KeryingService* on my machine).
int GetPbkdf2Iterations() {
  return KeyringService::GetPbkdf2IterationsForTesting().value_or(
      kPbkdf2Iterations);
}

const base::Value::List* GetPrefForKeyringList(const PrefService& profile_prefs,
                                               const std::string& key,
                                               mojom::KeyringId keyring_id) {
  if (const base::Value* result =
          KeyringService::GetPrefForKeyring(profile_prefs, key, keyring_id);
      result && result->is_list()) {
    return result->GetIfList();
  }
  return nullptr;
}

const base::Value::Dict* GetPrefForKeyringDict(const PrefService& profile_prefs,
                                               const std::string& key,
                                               mojom::KeyringId keyring_id) {
  if (const base::Value* result =
          KeyringService::GetPrefForKeyring(profile_prefs, key, keyring_id);
      result && result->is_dict()) {
    return result->GetIfDict();
  }
  return nullptr;
}

base::Value::List& GetListPrefForKeyringUpdate(
    ScopedDictPrefUpdate& dict_update,
    const std::string& key,
    mojom::KeyringId keyring_id) {
  return *dict_update.Get()
              .EnsureDict(KeyringIdPrefString(keyring_id))
              ->EnsureList(key);
}

base::Value::Dict& GetDictPrefForKeyringUpdate(
    ScopedDictPrefUpdate& dict_update,
    const std::string& key,
    mojom::KeyringId keyring_id) {
  return *dict_update.Get()
              .EnsureDict(KeyringIdPrefString(keyring_id))
              ->EnsureDict(key);
}

// Utility structure that helps storing imported accounts in prefs.
struct ImportedAccountInfo {
  ImportedAccountInfo(std::string account_name,
                      std::string account_address,
                      std::vector<uint8_t> encrypted_private_key)
      : account_name(std::move(account_name)),
        account_address(std::move(account_address)),
        encrypted_private_key(std::move(encrypted_private_key)) {}

  ~ImportedAccountInfo() = default;
  ImportedAccountInfo(const ImportedAccountInfo& other) = default;

  base::Value ToValue() const {
    base::Value::Dict imported_account;
    imported_account.Set(kAccountName, account_name);
    imported_account.Set(kAccountAddress, account_address);
    imported_account.Set(kEncryptedPrivateKey,
                         base::Base64Encode(encrypted_private_key));
    return base::Value(std::move(imported_account));
  }

  static absl::optional<ImportedAccountInfo> FromValue(
      const base::Value& value) {
    if (!value.is_dict()) {
      return absl::nullopt;
    }
    auto& value_dict = value.GetDict();

    const std::string* account_name = value_dict.FindString(kAccountName);
    const std::string* account_address = value_dict.FindString(kAccountAddress);
    const std::string* encrypted_private_key =
        value_dict.FindString(kEncryptedPrivateKey);
    if (!account_name || !account_address || !encrypted_private_key) {
      return absl::nullopt;
    }

    auto encrypted_private_key_decoded =
        base::Base64Decode(*encrypted_private_key);
    if (!encrypted_private_key_decoded) {
      return absl::nullopt;
    }

    return ImportedAccountInfo(*account_name, *account_address,
                               *encrypted_private_key_decoded);
  }

  std::string account_name;
  std::string account_address;
  std::vector<uint8_t> encrypted_private_key;
};

// Adds imported account to prefs.
void AddImportedAccountForKeyring(PrefService* profile_prefs,
                                  const ImportedAccountInfo& info,
                                  mojom::KeyringId keyring_id) {
  DCHECK(profile_prefs);

  ScopedDictPrefUpdate update(profile_prefs, kBraveWalletKeyrings);
  auto& imported_accounts =
      GetListPrefForKeyringUpdate(update, kImportedAccounts, keyring_id);

  imported_accounts.Append(info.ToValue());
}

// Gets all imported account from prefs.
std::vector<ImportedAccountInfo> GetImportedAccountsForKeyring(
    PrefService* profile_prefs,
    mojom::KeyringId keyring_id) {
  std::vector<ImportedAccountInfo> result;
  const base::Value::List* imported_accounts =
      GetPrefForKeyringList(*profile_prefs, kImportedAccounts, keyring_id);
  if (!imported_accounts) {
    return result;
  }
  for (const auto& item : *imported_accounts) {
    if (auto imported_account = ImportedAccountInfo::FromValue(item)) {
      result.emplace_back(std::move(*imported_account));
    }
  }
  return result;
}

// Removes imported account from prefs by address.
void RemoveImportedAccountForKeyring(PrefService* profile_prefs,
                                     const std::string& address,
                                     mojom::KeyringId keyring_id) {
  DCHECK(profile_prefs);

  ScopedDictPrefUpdate update(profile_prefs, kBraveWalletKeyrings);
  auto& imported_accounts =
      GetListPrefForKeyringUpdate(update, kImportedAccounts, keyring_id);

  imported_accounts.EraseIf([&](const base::Value& v) {
    if (auto* dict = v.GetIfDict()) {
      if (auto* account_address = dict->FindString(kAccountAddress)) {
        return *account_address == address;
      }
    }
    NOTREACHED();
    return false;
  });
}

// Utility structure that helps storing HD accounts in prefs.
struct DerivedAccountInfo {
  DerivedAccountInfo(uint32_t account_index,
                     std::string account_name,
                     std::string account_address)
      : account_index(account_index),
        account_name(std::move(account_name)),
        account_address(std::move(account_address)) {}

  ~DerivedAccountInfo() = default;
  DerivedAccountInfo(const DerivedAccountInfo& other) = default;

  base::Value ToValue() const {
    base::Value::Dict derived_account;
    derived_account.Set(kAccountIndex, base::NumberToString(account_index));
    derived_account.Set(kAccountName, account_name);
    derived_account.Set(kAccountAddress, account_address);
    return base::Value(std::move(derived_account));
  }

  static absl::optional<DerivedAccountInfo> FromValue(
      const base::Value& value) {
    auto* value_dict = value.GetIfDict();
    if (!value_dict) {
      return absl::nullopt;
    }
    const std::string* account_index_string =
        value_dict->FindString(kAccountIndex);
    const std::string* account_name = value_dict->FindString(kAccountName);
    const std::string* account_address =
        value_dict->FindString(kAccountAddress);
    if (!account_index_string || !account_name || !account_address) {
      return absl::nullopt;
    }

    uint32_t account_index = 0;
    if (!base::StringToUint(*account_index_string, &account_index)) {
      return absl::nullopt;
    }

    return DerivedAccountInfo(account_index, *account_name, *account_address);
  }

  uint32_t account_index;
  std::string account_name;
  std::string account_address;
};

// Gets all hd account from prefs.
std::vector<DerivedAccountInfo> GetDerivedAccountsForKeyring(
    PrefService* profile_prefs,
    mojom::KeyringId keyring_id) {
  const base::Value::List* derived_accounts =
      GetPrefForKeyringList(*profile_prefs, kAccountMetas, keyring_id);
  if (!derived_accounts) {
    return {};
  }

  std::vector<DerivedAccountInfo> result;
  for (auto& item : *derived_accounts) {
    if (auto derived_account = DerivedAccountInfo::FromValue(item)) {
      DCHECK_EQ(derived_account->account_index, result.size())
          << "No gaps allowed";
      result.emplace_back(std::move(*derived_account));
    }
  }

  return result;
}

size_t GetDerivedAccountsNumberForKeyring(PrefService* profile_prefs,
                                          mojom::KeyringId keyring_id) {
  return GetDerivedAccountsForKeyring(profile_prefs, keyring_id).size();
}

mojom::AccountIdPtr MakeAccountIdForDerivedAccount(
    const DerivedAccountInfo& derived_account_info,
    mojom::KeyringId keyring_id) {
  if (IsBitcoinKeyring(keyring_id)) {
    return MakeBitcoinAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                                mojom::AccountKind::kDerived,
                                derived_account_info.account_index);
  }

  return MakeAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                       mojom::AccountKind::kDerived,
                       derived_account_info.account_address);
}

mojom::AccountInfoPtr MakeAccountInfoForDerivedAccount(
    const DerivedAccountInfo& derived_account_info,
    mojom::KeyringId keyring_id) {
  // Ignore address from prefs for bitcoin.
  auto address =
      IsBitcoinKeyring(keyring_id) ? "" : derived_account_info.account_address;

  return mojom::AccountInfo::New(
      MakeAccountIdForDerivedAccount(derived_account_info, keyring_id),
      std::move(address), derived_account_info.account_name, nullptr);
}

mojom::AccountInfoPtr MakeAccountInfoForImportedAccount(
    const ImportedAccountInfo& imported_account_info,
    mojom::KeyringId keyring_id) {
  return mojom::AccountInfo::New(
      MakeAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                    mojom::AccountKind::kImported,
                    imported_account_info.account_address),
      imported_account_info.account_address, imported_account_info.account_name,
      nullptr);
}

void AddDerivedAccountInfoForKeyring(PrefService* profile_prefs,
                                     const DerivedAccountInfo& account,
                                     mojom::KeyringId keyring_id) {
  ScopedDictPrefUpdate keyrings_update(profile_prefs, kBraveWalletKeyrings);
  base::Value::List& account_metas =
      GetListPrefForKeyringUpdate(keyrings_update, kAccountMetas, keyring_id);
  DCHECK_EQ(account.account_index, account_metas.size()) << "No gaps allowed";
  account_metas.Append(account.ToValue());
}

bool SetSelectedWalletAccountInPrefs(PrefService* profile_prefs,
                                     const std::string& unique_key) {
  if (unique_key ==
      profile_prefs->GetString(kBraveWalletSelectedWalletAccount)) {
    return false;
  }

  profile_prefs->SetString(kBraveWalletSelectedWalletAccount, unique_key);
  return true;
}

std::string GetSelectedWalletAccountFromPrefs(PrefService* profile_prefs) {
  return profile_prefs->GetString(kBraveWalletSelectedWalletAccount);
}

bool SetSelectedDappAccountInPrefs(PrefService* profile_prefs,
                                   mojom::CoinType dapp_coin,
                                   const std::string& unique_key) {
  CHECK(CoinSupportsDapps(dapp_coin));
  const char* pref_name = dapp_coin == mojom::CoinType::ETH
                              ? kBraveWalletSelectedEthDappAccount
                              : kBraveWalletSelectedSolDappAccount;
  if (unique_key == profile_prefs->GetString(pref_name)) {
    return false;
  }

  profile_prefs->SetString(pref_name, unique_key);
  return true;
}

std::string GetSelectedDappAccountFromPrefs(PrefService* profile_prefs,
                                            mojom::CoinType dapp_coin) {
  CHECK(CoinSupportsDapps(dapp_coin));
  const char* pref_name = dapp_coin == mojom::CoinType::ETH
                              ? kBraveWalletSelectedEthDappAccount
                              : kBraveWalletSelectedSolDappAccount;

  return profile_prefs->GetString(pref_name);
}

}  // namespace

KeyringService::KeyringService(JsonRpcService* json_rpc_service,
                               PrefService* profile_prefs,
                               PrefService* local_state)
    : json_rpc_service_(json_rpc_service),
      profile_prefs_(profile_prefs),
      local_state_(local_state) {
  DCHECK(profile_prefs);
  DCHECK(local_state);
  auto_lock_timer_ = std::make_unique<base::OneShotTimer>();

  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(profile_prefs);
  pref_change_registrar_->Add(
      kBraveWalletAutoLockMinutes,
      base::BindRepeating(&KeyringService::OnAutoLockPreferenceChanged,
                          base::Unretained(this)));

  // Added 06/2023
  MaybeMigrateSelectedAccountPrefs();

  MaybeUnlockWithCommandLine();
}

KeyringService::~KeyringService() {
  auto_lock_timer_.reset();
}

// static
void KeyringService::MigrateDerivedAccountIndex(PrefService* profile_prefs) {
  ScopedDictPrefUpdate update(profile_prefs, kBraveWalletKeyrings);

  const std::vector<mojom::KeyringId> keyrings = {
      mojom::KeyringId::kDefault,   mojom::KeyringId::kSolana,
      mojom::KeyringId::kFilecoin,  mojom::KeyringId::kFilecoinTestnet,
      mojom::KeyringId::kBitcoin84, mojom::KeyringId::kBitcoin84Testnet};

  for (auto keyring_id : keyrings) {
    base::Value::Dict* keyring_dict =
        update->FindDict(KeyringIdPrefString(keyring_id));
    if (!keyring_dict) {
      continue;
    }

    base::Value::Dict* account_metas_dict =
        keyring_dict->FindDict(kAccountMetas);

    if (!account_metas_dict) {
      continue;
    }

    if (IsBitcoinKeyring(keyring_id)) {
      // Don't bother with migrating bitcoin accounts.
      account_metas_dict->clear();
    }

    std::map<uint32_t, base::Value::Dict> new_accounts_map;
    for (auto acc_item : *account_metas_dict) {
      auto account_index = ExtractAccountIndex(keyring_id, acc_item.first);
      if (!account_index) {
        NOTREACHED() << acc_item.first;
        continue;
      }
      if (!acc_item.second.is_dict()) {
        NOTREACHED();
        continue;
      }

      base::Value::Dict new_account = acc_item.second.GetIfDict()->Clone();
      new_account.Set(kAccountIndex, base::NumberToString(*account_index));
      new_accounts_map[*account_index] = std::move(new_account);
    }

    base::Value::List new_accounts;
    for (auto& acc : new_accounts_map) {
      new_accounts.Append(std::move(acc.second));
    }

    keyring_dict->Set(kAccountMetas, std::move(new_accounts));
  }
}

// static
absl::optional<int>& KeyringService::GetPbkdf2IterationsForTesting() {
  static absl::optional<int> iterations;
  return iterations;
}

mojo::PendingRemote<mojom::KeyringService> KeyringService::MakeRemote() {
  mojo::PendingRemote<mojom::KeyringService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void KeyringService::Bind(
    mojo::PendingReceiver<mojom::KeyringService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

// static
absl::optional<mojom::KeyringId> KeyringService::GetKeyringIdForCoinNonFIL(
    mojom::CoinType coin) {
  // TODO(apaymyshev): we should get rid of these methods which try to guess
  // keyring_id by coin as this is not possible for filecoin and bitcoin. In
  // many cases keyring_id should come as a call argument known from context.
  DCHECK_NE(coin, mojom::CoinType::BTC) << "Bitcoin not supported";

  if (coin == mojom::CoinType::FIL) {
    return absl::nullopt;
  } else if (coin == mojom::CoinType::SOL) {
    return mojom::kSolanaKeyringId;
  }

  DCHECK_EQ(coin, mojom::CoinType::ETH);
  return mojom::kDefaultKeyringId;
}

void KeyringService::MaybeMigrateSelectedAccountPrefs() {
  if (!profile_prefs_->HasPrefPath(kBraveWalletSelectedCoinDeprecated)) {
    return;
  }

  auto all_accounts = GetAllAccountInfos();
  if (all_accounts.empty()) {
    return;
  }

  constexpr char kSelectedAccountDeprecated[] = "selected_account";

  auto find_account = [&](mojom::KeyringId keyring_id) -> mojom::AccountIdPtr {
    if (!profile_prefs_->GetDict(kBraveWalletKeyrings)
             .FindDict(KeyringIdPrefString(keyring_id))) {
      return nullptr;
    }

    std::string address;
    {
      ScopedDictPrefUpdate update(profile_prefs_, kBraveWalletKeyrings);
      auto extracted = update->EnsureDict(KeyringIdPrefString(keyring_id))
                           ->Extract(kSelectedAccountDeprecated);
      if (extracted && extracted->is_string()) {
        address = extracted->GetString();
      }
    }

    if (address.empty()) {
      return nullptr;
    }

    for (auto& acc : all_accounts) {
      if (acc->account_id->keyring_id == keyring_id &&
          base::EqualsCaseInsensitiveASCII(acc->address, address)) {
        return acc->account_id.Clone();
      }
    }

    return nullptr;
  };

  mojom::AccountIdPtr eth_selected = find_account(mojom::KeyringId::kDefault);
  mojom::AccountIdPtr sol_selected = find_account(mojom::KeyringId::kSolana);
  mojom::AccountIdPtr fil_selected = find_account(mojom::KeyringId::kFilecoin);

  SetSelectedDappAccountInPrefs(profile_prefs_, mojom::CoinType::ETH,
                                eth_selected ? eth_selected->unique_key : "");
  SetSelectedDappAccountInPrefs(profile_prefs_, mojom::CoinType::SOL,
                                sol_selected ? sol_selected->unique_key : "");

  mojom::AccountIdPtr wallet_selected;
  auto coin = static_cast<mojom::CoinType>(
      profile_prefs_->GetInteger(kBraveWalletSelectedCoinDeprecated));
  switch (coin) {
    case mojom::CoinType::ETH:
      wallet_selected = eth_selected.Clone();
      break;
    case mojom::CoinType::SOL:
      wallet_selected = sol_selected.Clone();
      break;
    case mojom::CoinType::FIL:
      wallet_selected = fil_selected.Clone();
      break;
    case mojom::CoinType::BTC:
      NOTREACHED();
      break;
  }

  if (!wallet_selected) {
    wallet_selected = all_accounts.front()->account_id->Clone();
    DCHECK_EQ(mojom::CoinType::ETH, wallet_selected->coin);
  }
  SetSelectedWalletAccountInPrefs(profile_prefs_, wallet_selected->unique_key);
  profile_prefs_->ClearPref(kBraveWalletSelectedCoinDeprecated);
}

// static
void KeyringService::MigrateObsoleteProfilePrefs(PrefService* profile_prefs) {
  // Moving hardware part under default keyring.
  ScopedDictPrefUpdate update(profile_prefs, kBraveWalletKeyrings);
  auto* obsolete = update->FindDict(kHardwareAccounts);
  if (obsolete) {
    SetPrefForKeyring(profile_prefs, kHardwareAccounts,
                      base::Value(obsolete->Clone()), mojom::kDefaultKeyringId);
    update->Remove(kHardwareAccounts);
  }
}

// static
bool KeyringService::HasPrefForKeyring(const PrefService& profile_prefs,
                                       const std::string& key,
                                       mojom::KeyringId keyring_id) {
  return GetPrefForKeyring(profile_prefs, key, keyring_id) != nullptr;
}

// static
const base::Value* KeyringService::GetPrefForKeyring(
    const PrefService& profile_prefs,
    const std::string& key,
    mojom::KeyringId keyring_id) {
  const auto& keyrings_pref = profile_prefs.GetDict(kBraveWalletKeyrings);
  const base::Value::Dict* keyring_dict =
      keyrings_pref.FindDict(KeyringIdPrefString(keyring_id));
  if (!keyring_dict) {
    return nullptr;
  }

  return keyring_dict->Find(key);
}

// static
void KeyringService::SetPrefForKeyring(PrefService* profile_prefs,
                                       const std::string& key,
                                       base::Value value,
                                       mojom::KeyringId keyring_id) {
  DCHECK(profile_prefs);
  ScopedDictPrefUpdate update(profile_prefs, kBraveWalletKeyrings);
  update->EnsureDict(KeyringIdPrefString(keyring_id))
      ->Set(key, std::move(value));
}

HDKeyring* KeyringService::CreateKeyring(mojom::KeyringId keyring_id,
                                         const std::string& mnemonic,
                                         const std::string& password) {
  if (!mojom::IsKnownEnumValue(keyring_id)) {
    VLOG(1) << "Unknown keyring id " << keyring_id;
    return nullptr;
  }

  if (!CreateEncryptorForKeyring(password, keyring_id)) {
    return nullptr;
  }

  auto* keyring = CreateKeyringInternal(keyring_id, mnemonic, false);
  if (!keyring) {
    return nullptr;
  }

  for (const auto& observer : observers_) {
    observer->KeyringCreated(keyring_id);
  }
  ResetAutoLockTimer();

  return keyring;
}

void KeyringService::RequestUnlock() {
  DCHECK(IsLockedSync());
  request_unlock_pending_ = true;
}

HDKeyring* KeyringService::ResumeKeyring(mojom::KeyringId keyring_id,
                                         const std::string& password) {
  DCHECK(profile_prefs_);
  if (!CreateEncryptorForKeyring(password, keyring_id)) {
    return nullptr;
  }

  const std::string mnemonic = GetMnemonicForKeyringImpl(keyring_id);
  if (mnemonic.empty()) {
    return nullptr;
  }

  bool is_legacy_brave_wallet = false;
  if (auto* value =
          GetPrefForKeyring(*profile_prefs_, kLegacyBraveWallet, keyring_id)) {
    is_legacy_brave_wallet = value->GetBool();
  }
  auto* keyring =
      CreateKeyringInternal(keyring_id, mnemonic, is_legacy_brave_wallet);
  if (!keyring) {
    return nullptr;
  }

  size_t account_no =
      GetDerivedAccountsNumberForKeyring(profile_prefs_, keyring_id);
  if (account_no) {
    keyring->AddAccounts(account_no);
  }

  for (const auto& imported_account_info :
       GetImportedAccountsForKeyring(profile_prefs_, keyring_id)) {
    auto private_key = encryptors_[keyring_id]->Decrypt(
        base::make_span(imported_account_info.encrypted_private_key),
        GetOrCreateNonceForKeyring(keyring_id));
    if (!private_key) {
      continue;
    }

    if (IsFilecoinKeyringId(keyring_id)) {
      auto* filecoin_keyring = static_cast<FilecoinKeyring*>(keyring);
      if (filecoin_keyring) {
        filecoin_keyring->RestoreFilecoinAccount(
            *private_key, imported_account_info.account_address);
      }
    } else {
      keyring->ImportAccount(*private_key);
    }
  }

  return keyring;
}

HDKeyring* KeyringService::RestoreKeyring(mojom::KeyringId keyring_id,
                                          const std::string& mnemonic,
                                          const std::string& password,
                                          bool is_legacy_brave_wallet) {
  DCHECK(profile_prefs_);
  if (!IsValidMnemonic(mnemonic)) {
    return nullptr;
  }
  // Try getting existing mnemonic first
  if (CreateEncryptorForKeyring(password, keyring_id)) {
    const std::string current_mnemonic = GetMnemonicForKeyringImpl(keyring_id);
    // Restore with same mnmonic and same password, resume current keyring
    // Also need to make sure is_legacy_brave_wallet are the same, users might
    // choose the option wrongly and then want to start over with same mnemonic
    // but different is_legacy_brave_wallet value
    const base::Value* value =
        GetPrefForKeyring(*profile_prefs_, kLegacyBraveWallet, keyring_id);
    if (!current_mnemonic.empty() && current_mnemonic == mnemonic && value &&
        value->GetBool() == is_legacy_brave_wallet) {
      return ResumeKeyring(keyring_id, password);
    } else if (keyring_id == mojom::kDefaultKeyringId) {
      // We have no way to check if new mnemonic is same as current mnemonic so
      // we need to clear all profile_prefs for fresh start
      Reset(false);
      // Consider no migration needed after wallet is reset.
      profile_prefs_->SetBoolean(kBraveWalletKeyringEncryptionKeysMigrated,
                                 true);
    }
  }

  if (!CreateEncryptorForKeyring(password, keyring_id)) {
    return nullptr;
  }

  auto* keyring =
      CreateKeyringInternal(keyring_id, mnemonic, is_legacy_brave_wallet);
  if (!keyring) {
    return nullptr;
  }

  for (const auto& observer : observers_) {
    observer->KeyringRestored(keyring_id);
  }
  ResetAutoLockTimer();
  return keyring;
}

mojom::KeyringInfoPtr KeyringService::GetKeyringInfoSync(
    mojom::KeyringId keyring_id) {
  DCHECK(profile_prefs_);
  mojom::KeyringInfoPtr keyring_info = mojom::KeyringInfo::New();
  keyring_info->id = keyring_id;
  keyring_info->is_keyring_created = IsKeyringCreated(keyring_id);
  keyring_info->is_locked =
      !keyring_info->is_keyring_created || IsLocked(keyring_id);
  bool backup_complete = false;
  const base::Value* value =
      GetPrefForKeyring(*profile_prefs_, kBackupComplete, keyring_id);
  if (value) {
    backup_complete = value->GetBool();
  }
  keyring_info->is_backed_up = backup_complete;
  keyring_info->account_infos = GetAccountInfosForKeyring(keyring_id);
  return keyring_info;
}

void KeyringService::GetKeyringInfo(mojom::KeyringId keyring_id,
                                    GetKeyringInfoCallback callback) {
  std::move(callback).Run(GetKeyringInfoSync(keyring_id));
}

void KeyringService::GetMnemonicForDefaultKeyring(
    const std::string& password,
    GetMnemonicForDefaultKeyringCallback callback) {
  if (!ValidatePasswordInternal(password)) {
    std::move(callback).Run("");
    return;
  }

  std::move(callback).Run(GetMnemonicForKeyringImpl(mojom::kDefaultKeyringId));
}

void KeyringService::MaybeCreateDefaultSolanaAccount() {
  if (!ShouldCreateDefaultSolanaAccount()) {
    return;
  }
  if (!LazilyCreateKeyring(mojom::kSolanaKeyringId)) {
    return;
  }

  auto account = AddAccountForKeyring(mojom::kSolanaKeyringId,
                                      "Solana " + GetAccountName(1));
  if (account) {
    SetSelectedAccountInternal(*account);
    NotifyAccountsAdded(*account);
  }
}

void KeyringService::CreateWallet(const std::string& password,
                                  CreateWalletCallback callback) {
  profile_prefs_->SetBoolean(kBraveWalletKeyringEncryptionKeysMigrated, true);

  const std::string mnemonic = GenerateMnemonic(16);

  auto* keyring = CreateKeyring(mojom::kDefaultKeyringId, mnemonic, password);
  if (keyring) {
    const auto account =
        AddAccountForKeyring(mojom::kDefaultKeyringId, GetAccountName(1));
    if (account) {
      SetSelectedAccountInternal(*account);
      NotifyAccountsAdded(*account);
    }
  }

  // keep encryptor pre-created
  // to be able to lazily create keyring later
  if (IsFilecoinEnabled()) {
    if (!CreateEncryptorForKeyring(password, mojom::kFilecoinKeyringId)) {
      VLOG(1) << "Unable to create filecoin encryptor";
    }
    if (!CreateEncryptorForKeyring(password,
                                   mojom::kFilecoinTestnetKeyringId)) {
      VLOG(1) << "Unable to create filecoin testnet encryptor";
    }
  }
  if (IsSolanaEnabled()) {
    if (!CreateEncryptorForKeyring(password, mojom::kSolanaKeyringId)) {
      VLOG(1) << "Unable to create solana encryptor";
    }
    MaybeCreateDefaultSolanaAccount();
  }

  if (IsBitcoinEnabled()) {
    CreateKeyring(mojom::kBitcoinKeyring84Id, mnemonic, password);
    CreateKeyring(mojom::kBitcoinKeyring84TestId, mnemonic, password);
  }

  std::move(callback).Run(mnemonic);
}

void KeyringService::RestoreWallet(const std::string& mnemonic,
                                   const std::string& password,
                                   bool is_legacy_brave_wallet,
                                   RestoreWalletCallback callback) {
  auto* keyring = RestoreKeyring(mojom::kDefaultKeyringId, mnemonic, password,
                                 is_legacy_brave_wallet);
  if (keyring && !GetDerivedAccountsNumberForKeyring(
                     profile_prefs_, mojom::kDefaultKeyringId)) {
    const auto account =
        AddAccountForKeyring(mojom::kDefaultKeyringId, GetAccountName(1));
    if (account) {
      SetSelectedAccountInternal(*account);
      NotifyAccountsAdded(*account);
    }
  }

  if (IsFilecoinEnabled()) {
    // Restore mainnet filecoin acc
    RestoreKeyring(mojom::kFilecoinKeyringId, mnemonic, password, false);
    // Restore testnet filecoin acc
    RestoreKeyring(mojom::kFilecoinTestnetKeyringId, mnemonic, password, false);
  }

  if (IsSolanaEnabled()) {
    auto* solana_keyring =
        RestoreKeyring(mojom::kSolanaKeyringId, mnemonic, password, false);
    if (solana_keyring && !GetDerivedAccountsNumberForKeyring(
                              profile_prefs_, mojom::kSolanaKeyringId)) {
      MaybeCreateDefaultSolanaAccount();
    }
  }

  if (IsBitcoinEnabled()) {
    RestoreKeyring(mojom::kBitcoinKeyring84Id, mnemonic, password, false);
    RestoreKeyring(mojom::kBitcoinKeyring84TestId, mnemonic, password, false);
  }

  account_discovery_manager_ =
      std::make_unique<AccountDiscoveryManager>(json_rpc_service_, this);
  account_discovery_manager_->StartDiscovery();

  std::move(callback).Run(keyring);
}

std::string KeyringService::GetMnemonicForKeyringImpl(
    mojom::KeyringId keyring_id) {
  if (IsLocked(keyring_id) || !IsKeyringCreated(keyring_id)) {
    VLOG(1) << __func__ << ": Must Unlock service or create keyring first";
    return std::string();
  }
  DCHECK(encryptors_[keyring_id]);
  auto encrypted_mnemonic =
      GetPrefInBytesForKeyring(*profile_prefs_, kEncryptedMnemonic, keyring_id);
  if (!encrypted_mnemonic) {
    return std::string();
  }

  auto mnemonic = encryptors_[keyring_id]->Decrypt(
      *encrypted_mnemonic, GetOrCreateNonceForKeyring(keyring_id));
  if (!mnemonic) {
    return std::string();
  }

  return std::string(mnemonic->begin(), mnemonic->end());
}

void KeyringService::AddAccount(mojom::CoinType coin,
                                mojom::KeyringId keyring_id,
                                const std::string& account_name,
                                AddAccountCallback callback) {
  std::move(callback).Run(AddAccountSync(coin, keyring_id, account_name));
}

mojom::AccountInfoPtr KeyringService::AddAccountSync(
    mojom::CoinType coin,
    mojom::KeyringId keyring_id,
    const std::string& account_name) {
  if (IsFilecoinKeyringId(keyring_id)) {
    if (!IsFilecoinEnabled()) {
      return nullptr;
    }
    if (!LazilyCreateKeyring(keyring_id)) {
      VLOG(1) << "Unable to create Filecoin keyring";
      return nullptr;
    }
  }

  if (keyring_id == mojom::kSolanaKeyringId) {
    if (!IsSolanaEnabled()) {
      return nullptr;
    }
    if (!LazilyCreateKeyring(mojom::kSolanaKeyringId)) {
      VLOG(1) << "Unable to create Solana keyring";
      return nullptr;
    }
  }

  if (IsBitcoinKeyring(keyring_id)) {
    if (!IsBitcoinEnabled()) {
      return nullptr;
    }
  }

  auto* keyring = GetHDKeyringById(keyring_id);
  if (!keyring) {
    return nullptr;
  }
  auto account = AddAccountForKeyring(keyring_id, account_name);
  if (!account) {
    return nullptr;
  }

  // TODO(apaymyshev): ui should select account after creating account.
  SetSelectedAccountInternal(*account);
  NotifyAccountsAdded(*account);

  NotifyAccountsChanged();
  return account;
}

void KeyringService::EncodePrivateKeyForExport(
    mojom::AccountIdPtr account_id,
    const std::string& password,
    EncodePrivateKeyForExportCallback callback) {
  if (!account_id || !ValidatePasswordInternal(password)) {
    std::move(callback).Run("");
    return;
  }

  auto* keyring = GetHDKeyringById(account_id->keyring_id);
  if (!keyring) {
    std::move(callback).Run("");
    return;
  }

  std::move(callback).Run(
      keyring->EncodePrivateKeyForExport(account_id->address));
}

bool KeyringService::IsKeyringExist(mojom::KeyringId keyring_id) const {
  return keyrings_.contains(keyring_id) || IsKeyringCreated(keyring_id);
}

void KeyringService::ImportFilecoinAccount(
    const std::string& account_name,
    const std::string& private_key_hex,
    const std::string& network,
    ImportFilecoinAccountCallback callback) {
  const mojom::KeyringId filecoin_keyring_id = GetFilecoinKeyringId(network);
  if (!LazilyCreateKeyring(filecoin_keyring_id)) {
    std::move(callback).Run({});
    VLOG(1) << "Unable to create Filecoin keyring";
    return;
  }

  if (account_name.empty() || private_key_hex.empty() ||
      !encryptors_[filecoin_keyring_id]) {
    std::move(callback).Run({});
    return;
  }

  std::vector<uint8_t> private_key;
  mojom::FilecoinAddressProtocol protocol;
  if (!FilecoinKeyring::DecodeImportPayload(private_key_hex, &private_key,
                                            &protocol)) {
    std::move(callback).Run({});
    return;
  }

  auto* keyring =
      static_cast<FilecoinKeyring*>(GetHDKeyringById(filecoin_keyring_id));

  if (!keyring) {
    std::move(callback).Run({});
    return;
  }

  const std::string address =
      keyring->ImportFilecoinAccount(private_key, protocol);
  if (address.empty()) {
    std::move(callback).Run({});
    return;
  }

  std::vector<uint8_t> encrypted_key =
      encryptors_[filecoin_keyring_id]->Encrypt(
          private_key, GetOrCreateNonceForKeyring(filecoin_keyring_id));

  ImportedAccountInfo imported_account_info(account_name, address,
                                            encrypted_key);

  AddImportedAccountForKeyring(profile_prefs_, imported_account_info,
                               filecoin_keyring_id);

  auto account_info = MakeAccountInfoForImportedAccount(imported_account_info,
                                                        filecoin_keyring_id);
  // TODO(apaymyshev): ui should select account after importing.
  SetSelectedAccountInternal(*account_info);

  NotifyAccountsChanged();
  NotifyAccountsAdded(*account_info);

  std::move(callback).Run(std::move(account_info));
}

void KeyringService::ImportAccount(const std::string& account_name,
                                   const std::string& private_key,
                                   mojom::CoinType coin,
                                   ImportAccountCallback callback) {
  DCHECK_NE(coin, mojom::CoinType::BTC) << "Bitcoin not supported";

  std::string private_key_trimmed;
  base::TrimString(private_key, " \n\t", &private_key_trimmed);
  auto keyring_id = GetKeyringIdForCoinNonFIL(coin);

  if (!keyring_id) {
    NOTREACHED() << "ImportFilecoinAccount must be used";
    std::move(callback).Run({});
    return;
  }

  if (account_name.empty() || private_key.empty() ||
      !encryptors_[*keyring_id]) {
    std::move(callback).Run({});
    return;
  }

  std::vector<uint8_t> private_key_bytes;
  if (*keyring_id == mojom::kDefaultKeyringId) {
    if (!base::HexStringToBytes(private_key_trimmed, &private_key_bytes)) {
      // try again with 0x prefix considered
      if (!PrefixedHexStringToBytes(private_key_trimmed, &private_key_bytes)) {
        std::move(callback).Run({});
        return;
      }
    }
  } else if (*keyring_id == mojom::kSolanaKeyringId) {
    if (!LazilyCreateKeyring(*keyring_id)) {
      VLOG(1) << "Unable to create Solana keyring";
      std::move(callback).Run({});
      return;
    }
    std::vector<uint8_t> keypair(kSolanaKeypairSize);
    if (!Base58Decode(private_key_trimmed, &keypair, keypair.size())) {
      if (!Uint8ArrayDecode(private_key_trimmed, &keypair,
                            kSolanaKeypairSize)) {
        std::move(callback).Run({});
        return;
      }
    }
    // extract private key from keypair
    private_key_bytes = std::move(keypair);
  }

  if (private_key_bytes.empty()) {
    std::move(callback).Run({});
    return;
  }

  auto account = ImportAccountForKeyring(coin, *keyring_id, account_name,
                                         private_key_bytes);

  std::move(callback).Run(std::move(account));
}

void KeyringService::ImportAccountFromJson(const std::string& account_name,
                                           const std::string& password,
                                           const std::string& json,
                                           ImportAccountCallback callback) {
  if (account_name.empty() || password.empty() || json.empty() ||
      !encryptors_[mojom::kDefaultKeyringId]) {
    std::move(callback).Run({});
    return;
  }
  std::unique_ptr<HDKey> hd_key = HDKey::GenerateFromV3UTC(password, json);
  if (!hd_key) {
    std::move(callback).Run({});
    return;
  }

  auto account =
      ImportAccountForKeyring(mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                              account_name, hd_key->GetPrivateKeyBytes());
  std::move(callback).Run(std::move(account));
}

HDKeyring* KeyringService::GetHDKeyringById(mojom::KeyringId keyring_id) const {
  if (keyrings_.contains(keyring_id)) {
    return keyrings_.at(keyring_id).get();
  }
  return nullptr;
}

BitcoinKeyring* KeyringService::GetBitcoinKeyringById(
    mojom::KeyringId keyring_id) const {
  if (!IsBitcoinKeyring(keyring_id)) {
    return nullptr;
  }

  return static_cast<BitcoinKeyring*>(GetHDKeyringById(keyring_id));
}

void KeyringService::SetSelectedAccountInternal(
    const mojom::AccountInfo& account_info) {
  SetSelectedWalletAccountInternal(account_info);

  if (CoinSupportsDapps(account_info.account_id->coin)) {
    SetSelectedDappAccountInternal(account_info.account_id->coin,
                                   account_info.Clone());
  }
}

void KeyringService::SetSelectedWalletAccountInternal(
    const mojom::AccountInfo& account_info) {
  const auto& account_id = *account_info.account_id;

  if (SetSelectedWalletAccountInPrefs(profile_prefs_, account_id.unique_key)) {
    NotifySelectedWalletAccountChanged(account_info);

    // TODO(apaymyshev): this should not be a part of KeyringService.
    if (account_id.coin == mojom::CoinType::FIL) {
      json_rpc_service_->SetNetwork(
          account_id.keyring_id == mojom::kFilecoinKeyringId
              ? mojom::kFilecoinMainnet
              : mojom::kFilecoinTestnet,
          account_id.coin, absl::nullopt);
    }
  }
}

void KeyringService::SetSelectedDappAccountInternal(
    mojom::CoinType coin,
    const mojom::AccountInfoPtr& account_info) {
  CHECK(CoinSupportsDapps(coin));
  CHECK(!account_info || account_info->account_id->coin == coin);

  if (SetSelectedDappAccountInPrefs(
          profile_prefs_, coin,
          account_info ? account_info->account_id->unique_key : "")) {
    NotifySelectedDappAccountChanged(coin, account_info);
  }
}

void KeyringService::RemoveAccount(mojom::AccountIdPtr account_id,
                                   const std::string& password,
                                   RemoveAccountCallback callback) {
  if (account_id->kind == mojom::AccountKind::kImported &&
      !ValidatePasswordInternal(password)) {
    std::move(callback).Run(false);
    return;
  }

  if (account_id->kind == mojom::AccountKind::kImported) {
    std::move(callback).Run(RemoveImportedAccountInternal(*account_id));
    return;
  }

  if (account_id->kind == mojom::AccountKind::kHardware) {
    std::move(callback).Run(RemoveHardwareAccountInternal(*account_id));
    return;
  }

  NOTREACHED();
  std::move(callback).Run(false);
}

bool KeyringService::RemoveImportedAccountInternal(
    const mojom::AccountId& account_id) {
  DCHECK_EQ(account_id.kind, mojom::AccountKind::kImported);
  auto* keyring = GetHDKeyringById(account_id.keyring_id);

  if (!keyring || !keyring->RemoveImportedAccount(account_id.address)) {
    return false;
  }

  RemoveImportedAccountForKeyring(profile_prefs_, account_id.address,
                                  account_id.keyring_id);
  MaybeFixAccountSelection();
  NotifyAccountsChanged();
  return true;
}

void KeyringService::IsWalletBackedUp(IsWalletBackedUpCallback callback) {
  bool backup_complete = false;
  const base::Value* value = GetPrefForKeyring(*profile_prefs_, kBackupComplete,
                                               mojom::kDefaultKeyringId);
  if (value) {
    backup_complete = value->GetBool();
  }
  std::move(callback).Run(backup_complete);
}

void KeyringService::NotifyWalletBackupComplete() {
  SetPrefForKeyring(profile_prefs_, kBackupComplete, base::Value(true),
                    mojom::kDefaultKeyringId);
  for (const auto& observer : observers_) {
    observer->BackedUp();
  }
}

mojom::AccountInfoPtr KeyringService::AddAccountForKeyring(
    mojom::KeyringId keyring_id,
    const std::string& account_name) {
  auto* keyring = GetHDKeyringById(keyring_id);
  if (!keyring) {
    return nullptr;
  }

  auto added_accounts = keyring->AddAccounts(1);
  if (added_accounts.empty()) {
    return nullptr;
  }
  DCHECK_EQ(added_accounts.size(), 1u);

  DerivedAccountInfo derived_account_info(
      added_accounts[0].account_index, account_name, added_accounts[0].address);

  AddDerivedAccountInfoForKeyring(profile_prefs_, derived_account_info,
                                  keyring_id);

  return MakeAccountInfoForDerivedAccount(derived_account_info, keyring_id);
}

mojom::AccountInfoPtr KeyringService::ImportAccountForKeyring(
    mojom::CoinType coin,
    mojom::KeyringId keyring_id,
    const std::string& account_name,
    const std::vector<uint8_t>& private_key) {
  auto* keyring = GetHDKeyringById(keyring_id);
  if (!keyring) {
    return {};
  }

  const std::string address = keyring->ImportAccount(private_key);
  if (address.empty()) {
    return {};
  }

  std::vector<uint8_t> encrypted_private_key = encryptors_[keyring_id]->Encrypt(
      private_key, GetOrCreateNonceForKeyring(keyring_id));

  ImportedAccountInfo imported_account_info(account_name, address,
                                            encrypted_private_key);
  AddImportedAccountForKeyring(profile_prefs_, imported_account_info,
                               keyring_id);

  auto account_info =
      MakeAccountInfoForImportedAccount(imported_account_info, keyring_id);

  // TODO(apaymyshev): ui should select account after importing.
  SetSelectedAccountInternal(*account_info);

  NotifyAccountsChanged();
  NotifyAccountsAdded(*account_info);
  return account_info;
}

// This member function should not assume that the wallet is unlocked!
std::vector<mojom::AccountInfoPtr> KeyringService::GetAccountInfosForKeyring(
    mojom::KeyringId keyring_id) const {
  std::vector<mojom::AccountInfoPtr> result;

  // Append HD accounts.
  for (const auto& derived_account_info :
       GetDerivedAccountsForKeyring(profile_prefs_, keyring_id)) {
    result.push_back(
        MakeAccountInfoForDerivedAccount(derived_account_info, keyring_id));
  }

  // Append imported accounts.
  for (const auto& imported_account_info :
       GetImportedAccountsForKeyring(profile_prefs_, keyring_id)) {
    result.push_back(
        MakeAccountInfoForImportedAccount(imported_account_info, keyring_id));
  }

  // Append hardware accounts.
  for (const auto& hardware_account_info :
       GetHardwareAccountsSync(keyring_id)) {
    result.push_back(hardware_account_info.Clone());
  }
  return result;
}

std::vector<mojom::AccountInfoPtr> KeyringService::GetHardwareAccountsSync(
    mojom::KeyringId keyring_id) const {
  std::vector<mojom::AccountInfoPtr> accounts;
  const base::Value::Dict* keyring =
      GetPrefForKeyringDict(*profile_prefs_, kHardwareAccounts, keyring_id);
  if (!keyring) {
    return accounts;
  }

  for (auto&& [id, value] : *keyring) {
    DCHECK(value.is_dict());
    const base::Value* account_value = value.GetDict().Find(kAccountMetas);
    if (!account_value) {
      continue;
    }
    SerializeHardwareAccounts(id, account_value, keyring_id, &accounts);
  }

  return accounts;
}

void KeyringService::AddHardwareAccounts(
    std::vector<mojom::HardwareWalletAccountPtr> infos,
    AddHardwareAccountsCallback callback) {
  return std::move(callback).Run(AddHardwareAccountsSync(std::move(infos)));
}

std::vector<mojom::AccountInfoPtr> KeyringService::AddHardwareAccountsSync(
    std::vector<mojom::HardwareWalletAccountPtr> infos) {
  if (infos.empty()) {
    return {};
  }

  ScopedDictPrefUpdate keyrings_update(profile_prefs_, kBraveWalletKeyrings);

  std::vector<mojom::AccountInfoPtr> accounts_added;
  for (const auto& info : infos) {
    const mojom::CoinType coin = info->coin;
    mojom::KeyringId keyring_id = info->keyring_id;
    const std::string& hardware_vendor = info->hardware_vendor;
    const std::string& device_id = info->device_id;

    base::Value::Dict hw_account;
    hw_account.Set(kAccountName, info->name);
    hw_account.Set(kHardwareVendor, hardware_vendor);
    hw_account.Set(kHardwareDerivationPath, info->derivation_path);
    hw_account.Set(kCoinType, static_cast<int>(coin));

    base::Value::Dict& hardware_keyrings = GetDictPrefForKeyringUpdate(
        keyrings_update, kHardwareAccounts, info->keyring_id);

    hardware_keyrings.EnsureDict(device_id)
        ->EnsureDict(kAccountMetas)
        ->Set(info->address, std::move(hw_account));

    auto account_info = MakeAccountInfoForHardwareAccount(
        info->address, info->name, info->derivation_path, hardware_vendor,
        info->device_id, keyring_id);

    accounts_added.push_back(std::move(account_info));
  }

  // TODO(apaymyshev): ui should select account after importing.
  SetSelectedAccountInternal(*accounts_added.front());
  NotifyAccountsChanged();
  NotifyAccountsAdded(accounts_added);

  return accounts_added;
}

bool KeyringService::RemoveHardwareAccountInternal(
    const mojom::AccountId& account_id) {
  ScopedDictPrefUpdate keyrings_update(profile_prefs_, kBraveWalletKeyrings);
  base::Value::Dict& hardware_keyrings = GetDictPrefForKeyringUpdate(
      keyrings_update, kHardwareAccounts, account_id.keyring_id);
  for (auto&& [id, device] : hardware_keyrings) {
    DCHECK(device.is_dict());
    base::Value::Dict* account_metas = device.GetDict().FindDict(kAccountMetas);
    if (!account_metas) {
      continue;
    }
    const base::Value* address_key = account_metas->Find(account_id.address);
    if (!address_key) {
      continue;
    }
    account_metas->Remove(account_id.address);

    if (account_metas->empty()) {
      hardware_keyrings.Remove(id);
    }

    MaybeFixAccountSelection();
    NotifyAccountsChanged();
    return true;
  }

  return false;
}

absl::optional<std::string> KeyringService::SignTransactionByFilecoinKeyring(
    FilTransaction* tx) {
  if (!tx) {
    return absl::nullopt;
  }

  mojom::KeyringId keyring_id = GetFilecoinKeyringId(tx->from().network());
  auto* keyring = GetHDKeyringById(keyring_id);
  if (!keyring) {
    return absl::nullopt;
  }
  return static_cast<FilecoinKeyring*>(keyring)->SignTransaction(tx);
}

absl::optional<std::string> KeyringService::GetDiscoveryAddress(
    mojom::KeyringId keyring_id,
    int index) {
  auto* hd_keyring = GetHDKeyringById(keyring_id);
  if (!hd_keyring) {
    return absl::nullopt;
  }
  return hd_keyring->GetDiscoveryAddress(index);
}

void KeyringService::SignTransactionByDefaultKeyring(const std::string& address,
                                                     EthTransaction* tx,
                                                     uint256_t chain_id) {
  auto* keyring = GetHDKeyringById(mojom::kDefaultKeyringId);
  if (!keyring) {
    return;
  }
  static_cast<EthereumKeyring*>(keyring)->SignTransaction(address, tx,
                                                          chain_id);
}

KeyringService::SignatureWithError::SignatureWithError() = default;
KeyringService::SignatureWithError::SignatureWithError(
    SignatureWithError&& other) = default;
KeyringService::SignatureWithError&
KeyringService::SignatureWithError::operator=(SignatureWithError&& other) =
    default;
KeyringService::SignatureWithError::~SignatureWithError() = default;

KeyringService::SignatureWithError KeyringService::SignMessageByDefaultKeyring(
    const std::string& address,
    const std::vector<uint8_t>& message,
    bool is_eip712) {
  SignatureWithError ret;
  auto* keyring = GetHDKeyringById(mojom::kDefaultKeyringId);
  if (!keyring) {
    ret.signature = absl::nullopt;
    ret.error_message =
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_UNLOCK_FIRST);
    return ret;
  }

  // MM currently doesn't provide chain_id when signing message
  std::vector<uint8_t> signature =
      static_cast<EthereumKeyring*>(keyring)->SignMessage(address, message, 0,
                                                          is_eip712);
  if (signature.empty()) {
    ret.signature = absl::nullopt;
    ret.error_message =
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_INVALID_ADDRESS,
                                  base::ASCIIToUTF16(address));
    return ret;
  }
  ret.signature = std::move(signature);
  return ret;
}

bool KeyringService::RecoverAddressByDefaultKeyring(
    const std::vector<uint8_t>& message,
    const std::vector<uint8_t>& signature,
    std::string* address) {
  CHECK(address);
  return EthereumKeyring::RecoverAddress(message, signature, address);
}

bool KeyringService::GetPublicKeyFromX25519_XSalsa20_Poly1305ByDefaultKeyring(
    const std::string& address,
    std::string* key) {
  CHECK(key);
  auto* keyring = GetHDKeyringById(mojom::kDefaultKeyringId);
  if (!keyring) {
    return false;
  }
  return static_cast<EthereumKeyring*>(keyring)
      ->GetPublicKeyFromX25519_XSalsa20_Poly1305(
          EthAddress::FromHex(address).ToChecksumAddress(), key);
}

absl::optional<std::vector<uint8_t>>
KeyringService::DecryptCipherFromX25519_XSalsa20_Poly1305ByDefaultKeyring(
    const std::string& version,
    const std::vector<uint8_t>& nonce,
    const std::vector<uint8_t>& ephemeral_public_key,
    const std::vector<uint8_t>& ciphertext,
    const std::string& address) {
  auto* keyring = GetHDKeyringById(mojom::kDefaultKeyringId);
  if (!keyring) {
    return absl::nullopt;
  }

  return static_cast<EthereumKeyring*>(keyring)
      ->DecryptCipherFromX25519_XSalsa20_Poly1305(
          version, nonce, ephemeral_public_key, ciphertext,
          EthAddress::FromHex(address).ToChecksumAddress());
}

std::vector<uint8_t> KeyringService::SignMessageBySolanaKeyring(
    const mojom::AccountId& account_id,
    const std::vector<uint8_t>& message) {
  auto* keyring =
      static_cast<SolanaKeyring*>(GetHDKeyringById(mojom::kSolanaKeyringId));
  if (!keyring) {
    return std::vector<uint8_t>();
  }

  return keyring->SignMessage(account_id.address, message);
}

void KeyringService::AddAccountsWithDefaultName(
    const mojom::CoinType& coin_type,
    mojom::KeyringId keyring_id,
    size_t number) {
  auto* keyring = GetHDKeyringById(keyring_id);
  if (!keyring) {
    NOTREACHED();
    return;
  }

  std::string prefix;
  if (coin_type == mojom::CoinType::FIL) {
    prefix = "Filecoin ";
  } else if (coin_type == mojom::CoinType::SOL) {
    prefix = "Solana ";
  }

  std::vector<mojom::AccountInfoPtr> account_infos;
  size_t current_num =
      GetDerivedAccountsNumberForKeyring(profile_prefs_, keyring_id);
  for (size_t i = current_num + 1; i <= current_num + number; ++i) {
    auto add_result = AddAccountForKeyring(
        keyring_id, base::StrCat({prefix, GetAccountName(i)}));
    if (add_result) {
      account_infos.push_back(std::move(add_result));
    }
  }
  NotifyAccountsChanged();
  NotifyAccountsAdded(account_infos);
}

bool KeyringService::IsLocked(mojom::KeyringId keyring_id) const {
  // It doesn't require password when the keyring is not yet created.
  if (!IsKeyringCreated(keyring_id)) {
    return false;
  }

  auto it = encryptors_.find(keyring_id);
  return (it == encryptors_.end()) || (it->second.get() == nullptr);
}

bool KeyringService::IsLockedSync() const {
  // If a user has both software and hardware wallets, leaning on always require
  // password despite hardware doesn’t need it.
  // If a user has only software wallet, same as above.
  // If a user has only hardware wallet, no password needed.
  return IsLocked(mojom::kDefaultKeyringId) ||
         IsLocked(mojom::kSolanaKeyringId) ||
         IsLocked(mojom::kFilecoinKeyringId) ||
         IsLocked(mojom::kFilecoinTestnetKeyringId);
}

bool KeyringService::HasPendingUnlockRequest() const {
  return request_unlock_pending_;
}

void KeyringService::Lock() {
  if (IsLockedSync()) {
    return;
  }

  keyrings_.clear();
  encryptors_.clear();

  for (const auto& observer : observers_) {
    observer->Locked();
  }
  StopAutoLockTimer();
}

bool KeyringService::IsHardwareAccount(mojom::KeyringId keyring_id,
                                       const std::string& address) const {
  for (const auto& hardware_account_info :
       GetHardwareAccountsSync(keyring_id)) {
    if (base::EqualsCaseInsensitiveASCII(hardware_account_info->address,
                                         address)) {
      return true;
    }
  }
  return false;
}

void KeyringService::Unlock(const std::string& password,
                            KeyringService::UnlockCallback callback) {
  if (!ResumeKeyring(mojom::kDefaultKeyringId, password)) {
    encryptors_.erase(mojom::kDefaultKeyringId);
    std::move(callback).Run(false);
    return;
  }

  if (IsFilecoinEnabled()) {
    if (!ResumeKeyring(mojom::kFilecoinKeyringId, password)) {
      // If Filecoin keyring doesnt exist we keep encryptor pre-created
      // to be able to lazily create keyring later
      if (IsKeyringExist(mojom::kFilecoinKeyringId)) {
        VLOG(1) << __func__ << " Unable to unlock filecoin keyring";
        encryptors_.erase(mojom::kFilecoinKeyringId);
        std::move(callback).Run(false);
        return;
      }
    }

    if (!ResumeKeyring(mojom::kFilecoinTestnetKeyringId, password)) {
      if (IsKeyringExist(mojom::kFilecoinTestnetKeyringId)) {
        VLOG(1) << __func__ << " Unable to unlock filecoin testnet keyring";
        encryptors_.erase(mojom::kFilecoinTestnetKeyringId);
        std::move(callback).Run(false);
        return;
      }
    }
  }

  if (IsSolanaEnabled() && !ResumeKeyring(mojom::kSolanaKeyringId, password)) {
    if (IsKeyringExist(mojom::kSolanaKeyringId)) {
      VLOG(1) << __func__ << " Unable to unlock Solana keyring";
      encryptors_.erase(mojom::kSolanaKeyringId);
      std::move(callback).Run(false);
      return;
    }
  }

  if (IsBitcoinEnabled()) {
    ResumeKeyring(mojom::kBitcoinKeyring84Id, password);
    ResumeKeyring(mojom::kBitcoinKeyring84TestId, password);
  }

  UpdateLastUnlockPref(local_state_);
  request_unlock_pending_ = false;
  for (const auto& observer : observers_) {
    observer->Unlocked();
  }
  ResetAutoLockTimer();

  std::move(callback).Run(true);
}

void KeyringService::OnAutoLockFired() {
  Lock();
}

void KeyringService::IsLocked(IsLockedCallback callback) {
  std::move(callback).Run(IsLockedSync());
}

void KeyringService::Reset(bool notify_observer) {
  account_discovery_manager_.reset();
  StopAutoLockTimer();
  encryptors_.clear();
  keyrings_.clear();
  ClearKeyringServiceProfilePrefs(profile_prefs_);
  if (notify_observer) {
    for (const auto& observer : observers_) {
      observer->KeyringReset();
    }
  }
}

void KeyringService::MaybeMigratePBKDF2Iterations(const std::string& password) {
  if (profile_prefs_->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated)) {
    return;
  }

  // Pref is supposed to be set only as true.
  DCHECK(
      !profile_prefs_->HasPrefPath(kBraveWalletKeyringEncryptionKeysMigrated));

  for (auto keyring_id :
       {mojom::kDefaultKeyringId, mojom::kFilecoinKeyringId,
        mojom::kFilecoinTestnetKeyringId, mojom::kSolanaKeyringId}) {
    auto legacy_encrypted_mnemonic = GetPrefInBytesForKeyring(
        *profile_prefs_, kEncryptedMnemonic, keyring_id);
    auto legacy_nonce = GetPrefInBytesForKeyring(
        *profile_prefs_, kPasswordEncryptorNonce, keyring_id);
    auto legacy_salt = GetPrefInBytesForKeyring(
        *profile_prefs_, kPasswordEncryptorSalt, keyring_id);

    if (!legacy_encrypted_mnemonic || !legacy_nonce || !legacy_salt) {
      continue;
    }

    auto legacy_encryptor = PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
        password, *legacy_salt, kPbkdf2IterationsLegacy, kPbkdf2KeySize);
    if (!legacy_encryptor) {
      continue;
    }

    auto mnemonic =
        legacy_encryptor->Decrypt(*legacy_encrypted_mnemonic, *legacy_nonce);
    if (!mnemonic) {
      continue;
    }

    auto salt = GetOrCreateSaltForKeyring(keyring_id, /*force_create = */ true);

    auto encryptor = PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
        password, salt, GetPbkdf2Iterations(), kPbkdf2KeySize);
    if (!encryptor) {
      continue;
    }

    auto nonce =
        GetOrCreateNonceForKeyring(keyring_id, /*force_create = */ true);

    SetPrefInBytesForKeyring(
        profile_prefs_, kEncryptedMnemonic,
        encryptor->Encrypt(base::make_span(*mnemonic), nonce), keyring_id);

    if (keyring_id == mojom::kDefaultKeyringId) {
      profile_prefs_->SetBoolean(kBraveWalletKeyringEncryptionKeysMigrated,
                                 true);
    }

    const base::Value::List* imported_accounts_legacy =
        GetPrefForKeyringList(*profile_prefs_, kImportedAccounts, keyring_id);
    if (!imported_accounts_legacy) {
      continue;
    }
    base::Value::List imported_accounts = imported_accounts_legacy->Clone();
    for (auto& imported_account : imported_accounts) {
      if (!imported_account.is_dict()) {
        continue;
      }

      const std::string* legacy_encrypted_private_key =
          imported_account.GetDict().FindString(kEncryptedPrivateKey);
      if (!legacy_encrypted_private_key) {
        continue;
      }

      auto legacy_private_key_decoded =
          base::Base64Decode(*legacy_encrypted_private_key);
      if (!legacy_private_key_decoded) {
        continue;
      }

      auto private_key = legacy_encryptor->Decrypt(
          base::make_span(*legacy_private_key_decoded), *legacy_nonce);
      if (!private_key) {
        continue;
      }

      imported_account.GetDict().Set(
          kEncryptedPrivateKey,
          base::Base64Encode(encryptor->Encrypt(*private_key, nonce)));
    }
    SetPrefForKeyring(profile_prefs_, kImportedAccounts,
                      base::Value(std::move(imported_accounts)), keyring_id);
  }
}

void KeyringService::StopAutoLockTimer() {
  auto_lock_timer_->Stop();
}

void KeyringService::ResetAutoLockTimer() {
  if (auto_lock_timer_->IsRunning()) {
    auto_lock_timer_->Reset();
  } else {
    size_t auto_lock_minutes =
        (size_t)profile_prefs_->GetInteger(kBraveWalletAutoLockMinutes);
    auto_lock_timer_->Start(FROM_HERE, base::Minutes(auto_lock_minutes), this,
                            &KeyringService::OnAutoLockFired);
  }
}

// static
absl::optional<std::vector<uint8_t>> KeyringService::GetPrefInBytesForKeyring(
    const PrefService& profile_prefs,
    const std::string& key,
    mojom::KeyringId keyring_id) {
  const base::Value* value = GetPrefForKeyring(profile_prefs, key, keyring_id);
  if (!value) {
    return absl::nullopt;
  }

  const std::string* encoded = value->GetIfString();
  if (!encoded || encoded->empty()) {
    return absl::nullopt;
  }

  return base::Base64Decode(*encoded);
}

// static
void KeyringService::SetPrefInBytesForKeyring(PrefService* profile_prefs,
                                              const std::string& key,
                                              base::span<const uint8_t> bytes,
                                              mojom::KeyringId keyring_id) {
  const std::string encoded = base::Base64Encode(bytes);
  SetPrefForKeyring(profile_prefs, key, base::Value(encoded), keyring_id);
}

std::vector<uint8_t> KeyringService::GetOrCreateNonceForKeyring(
    mojom::KeyringId keyring_id,
    bool force_create) {
  if (!force_create) {
    if (auto nonce = GetPrefInBytesForKeyring(
            *profile_prefs_, kPasswordEncryptorNonce, keyring_id)) {
      return *nonce;
    }
  }

  std::vector<uint8_t> nonce(kNonceSize);
  crypto::RandBytes(nonce);
  SetPrefInBytesForKeyring(profile_prefs_, kPasswordEncryptorNonce, nonce,
                           keyring_id);
  return nonce;
}

std::vector<uint8_t> KeyringService::GetOrCreateSaltForKeyring(
    mojom::KeyringId keyring_id,
    bool force_create) {
  if (!force_create) {
    if (auto salt = GetPrefInBytesForKeyring(
            *profile_prefs_, kPasswordEncryptorSalt, keyring_id)) {
      return *salt;
    }
  }

  std::vector<uint8_t> salt(kSaltSize);
  crypto::RandBytes(salt);
  SetPrefInBytesForKeyring(profile_prefs_, kPasswordEncryptorSalt, salt,
                           keyring_id);
  return salt;
}

bool KeyringService::CreateEncryptorForKeyring(const std::string& password,
                                               mojom::KeyringId keyring_id) {
  if (password.empty()) {
    return false;
  }

  // Added 08.08.2022
  MaybeMigratePBKDF2Iterations(password);

  encryptors_[keyring_id] = PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
      password, GetOrCreateSaltForKeyring(keyring_id), GetPbkdf2Iterations(),
      kPbkdf2KeySize);
  return encryptors_[keyring_id] != nullptr;
}

HDKeyring* KeyringService::CreateKeyringInternal(mojom::KeyringId keyring_id,
                                                 const std::string& mnemonic,
                                                 bool is_legacy_brave_wallet) {
  if (!encryptors_[keyring_id]) {
    return nullptr;
  }

  auto seed = is_legacy_brave_wallet ? MnemonicToEntropy(mnemonic)
                                     : MnemonicToSeed(mnemonic, "");
  if (!seed) {
    return nullptr;
  }
  if (is_legacy_brave_wallet && seed->size() != 32) {
    VLOG(1) << __func__
            << "mnemonic for legacy brave wallet must be 24 words which will "
               "produce 32 bytes seed";
    return nullptr;
  }

  std::vector<uint8_t> encrypted_mnemonic = encryptors_[keyring_id]->Encrypt(
      ToSpan(mnemonic), GetOrCreateNonceForKeyring(keyring_id));

  SetPrefInBytesForKeyring(profile_prefs_, kEncryptedMnemonic,
                           encrypted_mnemonic, keyring_id);
  SetPrefForKeyring(profile_prefs_, kLegacyBraveWallet,
                    base::Value(is_legacy_brave_wallet), keyring_id);

  if (keyring_id == mojom::kDefaultKeyringId) {
    keyrings_[mojom::kDefaultKeyringId] = std::make_unique<EthereumKeyring>();
  } else if (IsFilecoinKeyringId(keyring_id)) {
    keyrings_[keyring_id] =
        std::make_unique<FilecoinKeyring>(GetFilecoinChainId(keyring_id));
  } else if (keyring_id == mojom::kSolanaKeyringId) {
    keyrings_[mojom::kSolanaKeyringId] = std::make_unique<SolanaKeyring>();
  } else if (keyring_id == mojom::kBitcoinKeyring84Id) {
    keyrings_[mojom::kBitcoinKeyring84Id] =
        std::make_unique<BitcoinKeyring>(false);
  } else if (keyring_id == mojom::kBitcoinKeyring84TestId) {
    keyrings_[mojom::kBitcoinKeyring84TestId] =
        std::make_unique<BitcoinKeyring>(true);
  }
  auto* keyring = GetHDKeyringById(keyring_id);
  DCHECK(keyring) << "No HDKeyring for " << keyring_id;
  if (keyring) {
    // TODO(apaymyshev): Keyring creation is always followed by this method
    // call. Should be moved into ctor.
    keyring->ConstructRootHDKey(*seed, GetRootPath(keyring_id));
  }

  UpdateLastUnlockPref(local_state_);

  return keyring;
}

bool KeyringService::IsKeyringCreated(mojom::KeyringId keyring_id) const {
  return HasPrefForKeyring(*profile_prefs_, kEncryptedMnemonic, keyring_id);
}

void KeyringService::AddObserver(
    ::mojo::PendingRemote<mojom::KeyringServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

void KeyringService::NotifyUserInteraction() {
  if (auto_lock_timer_->IsRunning()) {
    auto_lock_timer_->Reset();
  }
}

bool KeyringService::LazilyCreateKeyring(mojom::KeyringId keyring_id) {
  if (keyring_id == mojom::kDefaultKeyringId) {
    return false;
  }
  if (IsKeyringExist(keyring_id)) {
    return true;
  }
  // we use same mnemonic from default keyring for non default keyrings
  auto mnemonic = GetMnemonicForKeyringImpl(mojom::kDefaultKeyringId);
  if (!CreateKeyringInternal(keyring_id, mnemonic, false)) {
    return false;
  }

  for (const auto& observer : observers_) {
    observer->KeyringCreated(keyring_id);
  }

  return true;
}

void KeyringService::GetAllAccounts(GetAllAccountsCallback callback) {
  std::move(callback).Run(GetAllAccountsSync());
}

mojom::AllAccountsInfoPtr KeyringService::GetAllAccountsSync() {
  return mojom::AllAccountsInfo::New(
      GetAllAccountInfos(), GetSelectedWalletAccount(),
      GetSelectedEthereumDappAccount(), GetSelectedSolanaDappAccount());
}

void KeyringService::SetSelectedAccount(mojom::AccountIdPtr account_id,
                                        SetSelectedAccountCallback callback) {
  std::move(callback).Run(SetSelectedAccountSync(std::move(account_id)));
}

bool KeyringService::SetSelectedAccountSync(mojom::AccountIdPtr account_id) {
  std::vector<mojom::AccountInfoPtr> infos =
      GetAccountInfosForKeyring(account_id->keyring_id);

  for (const mojom::AccountInfoPtr& info : infos) {
    if (info->account_id == account_id) {
      SetSelectedAccountInternal(*info);
      return true;
    }
  }

  return false;
}

void KeyringService::SetAccountName(mojom::AccountIdPtr account_id,
                                    const std::string& name,
                                    SetAccountNameCallback callback) {
  if (name.empty()) {
    std::move(callback).Run(false);
    return;
  }

  switch (account_id->kind) {
    case mojom::AccountKind::kDerived:
      std::move(callback).Run(
          SetKeyringDerivedAccountNameInternal(*account_id, name));
      return;
    case mojom::AccountKind::kImported:
      std::move(callback).Run(
          SetKeyringImportedAccountNameInternal(*account_id, name));
      return;
    case mojom::AccountKind::kHardware:
      std::move(callback).Run(
          SetHardwareAccountNameInternal(*account_id, name));
      return;
  }
  NOTREACHED();
  std::move(callback).Run(false);
}

bool KeyringService::SetKeyringDerivedAccountNameInternal(
    const mojom::AccountId& account_id,
    const std::string& name) {
  DCHECK(!name.empty());

  ScopedDictPrefUpdate keyrings_update(profile_prefs_, kBraveWalletKeyrings);
  base::Value::List& account_metas = GetListPrefForKeyringUpdate(
      keyrings_update, kAccountMetas, account_id.keyring_id);
  for (auto& item : account_metas) {
    if (auto derived_account = DerivedAccountInfo::FromValue(item)) {
      if (account_id == *MakeAccountIdForDerivedAccount(
                            *derived_account, account_id.keyring_id)) {
        derived_account->account_name = name;
        item = derived_account->ToValue();
        NotifyAccountsChanged();
        return true;
      }
    }
  }

  return false;
}

bool KeyringService::SetHardwareAccountNameInternal(
    const mojom::AccountId& account_id,
    const std::string& name) {
  DCHECK(!name.empty());

  ScopedDictPrefUpdate keyrings_update(profile_prefs_, kBraveWalletKeyrings);
  base::Value::Dict& hardware_keyrings = GetDictPrefForKeyringUpdate(
      keyrings_update, kHardwareAccounts, account_id.keyring_id);
  for (auto&& [id, device] : hardware_keyrings) {
    DCHECK(device.is_dict());
    base::Value::Dict* account_metas = device.GetDict().FindDict(kAccountMetas);
    if (!account_metas) {
      continue;
    }
    base::Value::Dict* address_key =
        account_metas->FindDict(account_id.address);
    if (!address_key) {
      continue;
    }
    address_key->Set(kAccountName, name);
    NotifyAccountsChanged();
    return true;
  }
  return false;
}

bool KeyringService::SetKeyringImportedAccountNameInternal(
    const mojom::AccountId& account_id,
    const std::string& name) {
  DCHECK(!name.empty());

  auto* keyring = GetHDKeyringById(account_id.keyring_id);
  if (!keyring) {
    return false;
  }

  base::Value::List imported_accounts;
  const base::Value::List* value = GetPrefForKeyringList(
      *profile_prefs_, kImportedAccounts, account_id.keyring_id);
  if (!value) {
    return false;
  }

  imported_accounts = value->Clone();

  for (auto& entry : imported_accounts) {
    DCHECK(entry.is_dict());
    base::Value::Dict& dict = entry.GetDict();
    const std::string* account_address = dict.FindString(kAccountAddress);
    if (account_address && *account_address == account_id.address) {
      dict.Set(kAccountName, name);
      SetPrefForKeyring(profile_prefs_, kImportedAccounts,
                        base::Value(std::move(imported_accounts)),
                        account_id.keyring_id);
      NotifyAccountsChanged();
      return true;
    }
  }
  return false;
}

void KeyringService::NotifyAccountsChanged() {
  for (const auto& observer : observers_) {
    observer->AccountsChanged();
  }
}

void KeyringService::NotifyAccountsAdded(
    const mojom::AccountInfo& added_account) {
  std::vector<mojom::AccountInfoPtr> added_accounts;
  added_accounts.push_back(added_account.Clone());
  NotifyAccountsAdded(added_accounts);
}

void KeyringService::NotifyAccountsAdded(
    const std::vector<mojom::AccountInfoPtr>& added_accounts) {
  for (const auto& observer : observers_) {
    std::vector<mojom::AccountInfoPtr> added_accounts_clone;
    for (auto& account : added_accounts) {
      added_accounts_clone.push_back(account->Clone());
    }
    observer->AccountsAdded(std::move(added_accounts_clone));
  }
}

void KeyringService::OnAutoLockPreferenceChanged() {
  StopAutoLockTimer();
  ResetAutoLockTimer();
  for (const auto& observer : observers_) {
    observer->AutoLockMinutesChanged();
  }
}

void KeyringService::NotifySelectedWalletAccountChanged(
    const mojom::AccountInfo& account) {
  for (const auto& observer : observers_) {
    observer->SelectedWalletAccountChanged(account.Clone());
  }
}

void KeyringService::NotifySelectedDappAccountChanged(
    mojom::CoinType coin,
    const mojom::AccountInfoPtr& account) {
  CHECK(CoinSupportsDapps(coin));

  for (const auto& observer : observers_) {
    observer->SelectedDappAccountChanged(coin, account.Clone());
  }
}

void KeyringService::GetAutoLockMinutes(GetAutoLockMinutesCallback callback) {
  std::move(callback).Run(
      profile_prefs_->GetInteger(kBraveWalletAutoLockMinutes));
}

void KeyringService::SetAutoLockMinutes(int32_t minutes,
                                        SetAutoLockMinutesCallback callback) {
  // Check bounds
  if (minutes < kAutoLockMinutesMin || minutes > kAutoLockMinutesMax) {
    std::move(callback).Run(false);
    return;
  }

  int32_t old_auto_lock_minutes =
      profile_prefs_->GetInteger(kBraveWalletAutoLockMinutes);
  if (minutes != old_auto_lock_minutes) {
    profile_prefs_->SetInteger(kBraveWalletAutoLockMinutes, minutes);
  }
  std::move(callback).Run(true);
}

void KeyringService::IsStrongPassword(const std::string& password,
                                      IsStrongPasswordCallback callback) {
  if (password.length() < 7) {
    std::move(callback).Run(false);
    return;
  }

  // Has at least one letter
  if (!RE2::PartialMatch(password, "[a-zA-Z]")) {
    std::move(callback).Run(false);
    return;
  }

  // Has at least one number
  if (!RE2::PartialMatch(password, "[0-9]")) {
    std::move(callback).Run(false);
    return;
  }

  // Has at least one non-alphanumeric character
  if (!RE2::PartialMatch(password, "[^0-9a-zA-Z]")) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(true);
}

bool KeyringService::ValidatePasswordInternal(const std::string& password) {
  if (password.empty()) {
    return false;
  }

  const mojom::KeyringId keyring_id = mojom::kDefaultKeyringId;

  auto salt = GetPrefInBytesForKeyring(*profile_prefs_, kPasswordEncryptorSalt,
                                       keyring_id);
  auto encrypted_mnemonic =
      GetPrefInBytesForKeyring(*profile_prefs_, kEncryptedMnemonic, keyring_id);
  auto nonce = GetPrefInBytesForKeyring(*profile_prefs_,
                                        kPasswordEncryptorNonce, keyring_id);

  if (!salt || !encrypted_mnemonic || !nonce) {
    return false;
  }

  auto iterations =
      profile_prefs_->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated)
          ? GetPbkdf2Iterations()
          : kPbkdf2IterationsLegacy;

  // TODO(apaymyshev): move this call(and other ones in this file) to
  // background thread.
  auto encryptor = PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
      password, *salt, iterations, kPbkdf2KeySize);

  if (!encryptor) {
    return false;
  }

  auto mnemonic = encryptor->Decrypt(*encrypted_mnemonic, *nonce);
  return mnemonic && !mnemonic->empty();
}

void KeyringService::ValidatePassword(const std::string& password,
                                      ValidatePasswordCallback callback) {
  std::move(callback).Run(ValidatePasswordInternal(password));
}

void KeyringService::GetChecksumEthAddress(
    const std::string& address,
    GetChecksumEthAddressCallback callback) {
  std::move(callback).Run(EthAddress::FromHex(address).ToChecksumAddress());
}

void KeyringService::HasPendingUnlockRequest(
    HasPendingUnlockRequestCallback callback) {
  std::move(callback).Run(HasPendingUnlockRequest());
}

absl::optional<std::vector<std::pair<std::string, mojom::BitcoinKeyIdPtr>>>
KeyringService::GetBitcoinAddresses(const mojom::AccountId& account_id) {
  CHECK(IsBitcoinAccount(account_id));

  auto* bitcoin_keyring = GetBitcoinKeyringById(account_id.keyring_id);
  if (!bitcoin_keyring) {
    return absl::nullopt;
  }

  // TODO(apaymyshev): store used addresses indexes in prefs.

  // TODO(apaymyshev): temporarily just return first 30 recieve and 20 change
  // addresses.
  std::vector<std::pair<std::string, mojom::BitcoinKeyIdPtr>> addresses;
  for (auto i = 0; i < 30; ++i) {
    auto key_id =
        mojom::BitcoinKeyId::New(account_id.bitcoin_account_index, 0, i);
    auto address = bitcoin_keyring->GetAddress(*key_id);
    if (!address) {
      return absl::nullopt;
    }
    addresses.emplace_back(*address, std::move(key_id));
  }
  for (auto i = 0; i < 20; ++i) {
    auto key_id =
        mojom::BitcoinKeyId::New(account_id.bitcoin_account_index, 1, i);
    auto address = bitcoin_keyring->GetAddress(*key_id);
    if (!address) {
      return absl::nullopt;
    }
    addresses.emplace_back(*address, std::move(key_id));
  }

  return addresses;
}

absl::optional<std::string> KeyringService::GetBitcoinAddress(
    const mojom::AccountId& account_id,
    const mojom::BitcoinKeyId& key_id) {
  CHECK(IsBitcoinAccount(account_id));
  CHECK_EQ(account_id.bitcoin_account_index, key_id.account);

  auto* bitcoin_keyring = GetBitcoinKeyringById(account_id.keyring_id);
  if (!bitcoin_keyring) {
    return absl::nullopt;
  }

  return bitcoin_keyring->GetAddress(key_id);
}

absl::optional<std::vector<uint8_t>> KeyringService::GetBitcoinPubkey(
    const mojom::AccountId& account_id,
    const mojom::BitcoinKeyId& key_id) {
  CHECK(IsBitcoinAccount(account_id));
  CHECK_EQ(account_id.bitcoin_account_index, key_id.account);

  auto* bitcoin_keyring = GetBitcoinKeyringById(account_id.keyring_id);
  if (!bitcoin_keyring) {
    return absl::nullopt;
  }

  return bitcoin_keyring->GetPubkey(key_id);
}

absl::optional<std::vector<uint8_t>>
KeyringService::SignMessageByBitcoinKeyring(
    const mojom::AccountId& account_id,
    const mojom::BitcoinKeyId& key_id,
    base::span<const uint8_t, 32> message) {
  CHECK(IsBitcoinAccount(account_id));
  CHECK_EQ(account_id.bitcoin_account_index, key_id.account);

  auto* bitcoin_keyring = GetBitcoinKeyringById(account_id.keyring_id);
  if (!bitcoin_keyring) {
    return absl::nullopt;
  }

  return bitcoin_keyring->SignMessage(key_id, message);
}

std::vector<mojom::AccountInfoPtr> KeyringService::GetAllAccountInfos() {
  std::vector<mojom::AccountInfoPtr> account_infos;
  for (const auto& keyring_id : GetSupportedKeyrings()) {
    for (auto& account_info : GetAccountInfosForKeyring(keyring_id)) {
      account_infos.push_back(std::move(account_info));
    }
  }
  return account_infos;
}

mojom::AccountInfoPtr KeyringService::GetSelectedWalletAccount() {
  auto account_infos = GetAllAccountInfos();
  auto unique_key =
      profile_prefs_->GetString(kBraveWalletSelectedWalletAccount);

  mojom::AccountInfoPtr first_account;
  for (auto& account_info : account_infos) {
    if (account_info->account_id->unique_key == unique_key) {
      return account_info->Clone();
    }
    if (!first_account) {
      first_account = account_info->Clone();
    }
  }
  return first_account;
}

mojom::AccountInfoPtr KeyringService::GetSelectedEthereumDappAccount() {
  return GetSelectedDappAccount(mojom::CoinType::ETH);
}

mojom::AccountInfoPtr KeyringService::GetSelectedSolanaDappAccount() {
  return GetSelectedDappAccount(mojom::CoinType::SOL);
}

mojom::AccountInfoPtr KeyringService::GetSelectedDappAccount(
    mojom::CoinType coin) {
  CHECK(CoinSupportsDapps(coin));

  const mojom::KeyringId keyring_id = coin == mojom::CoinType::ETH
                                          ? mojom::KeyringId::kDefault
                                          : mojom::KeyringId::kSolana;

  auto unique_key = GetSelectedDappAccountFromPrefs(profile_prefs_, coin);

  for (auto& account_info : GetAccountInfosForKeyring(keyring_id)) {
    if (account_info->account_id->unique_key == unique_key) {
      return account_info->Clone();
    }
  }

  return {};
}

void KeyringService::MaybeFixAccountSelection() {
  const auto account_infos = GetAllAccountInfos();
  if (account_infos.empty()) {
    NOTREACHED();
    return;
  }

  std::set<std::string> unique_keys;
  for (auto& acc : account_infos) {
    unique_keys.insert(acc->account_id->unique_key);
  }

  if (!unique_keys.contains(
          GetSelectedWalletAccountFromPrefs(profile_prefs_))) {
    // Legacy behavior when after removing selected account ui picks first
    // available account.
    DCHECK_EQ(account_infos[0]->account_id->coin, mojom::CoinType::ETH);
    SetSelectedAccountInternal(*account_infos[0]);
  }

  if (!unique_keys.contains(GetSelectedDappAccountFromPrefs(
          profile_prefs_, mojom::CoinType::ETH))) {
    // ETH dApp account appears to be removed. Forcedly clear ETH dApp account
    // selection.
    SetSelectedDappAccountInternal(mojom::CoinType::ETH, {});
  }

  if (!unique_keys.contains(GetSelectedDappAccountFromPrefs(
          profile_prefs_, mojom::CoinType::SOL))) {
    // SOL dApp account appears to be removed. Forcedly clear SOL dApp account
    // selection.
    SetSelectedDappAccountInternal(mojom::CoinType::SOL, {});
  }
}

absl::optional<size_t> KeyringService::GetAccountsNumber(
    mojom::KeyringId keyring_id) {
  auto* keyring = GetHDKeyringById(keyring_id);
  if (!keyring) {
    return absl::nullopt;
  }
  return keyring->GetAccounts().size();
}

void KeyringService::MaybeUnlockWithCommandLine() {
#if !defined(OFFICIAL_BUILD)
  std::string dev_wallet_password =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kDevWalletPassword);
  if (!dev_wallet_password.empty()) {
    Unlock(dev_wallet_password, base::DoNothing());
  }
#endif  // !defined(OFFICIAL_BUILD)
}

}  // namespace brave_wallet
