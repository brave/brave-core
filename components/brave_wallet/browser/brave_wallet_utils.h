/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_UTILS_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

class PrefService;
namespace base {
class Value;
}  // namespace base

class GURL;

namespace brave_wallet {

// Generate mnemonic from random entropy following BIP39.
// |entropy_size| should be specify in bytes
// If |entropy_size| is not in 16, 20, 24, 28, 32 range or allocation
// failure, the empty string will be returned.
std::string GenerateMnemonic(size_t entropy_size);
// Testing specific entropy
std::string GenerateMnemonicForTest(const std::vector<uint8_t>& entropy);
// Generate seed from mnemonic following BIP39.
// If allocation failed, it would return nullptr. Otherwise 512 bits seed will
// be returned.
std::unique_ptr<std::vector<uint8_t>> MnemonicToSeed(
    const std::string& mnemonic,
    const std::string& passphrase);
// This is mainly used for restoring legacy brave crypto wallet
std::unique_ptr<std::vector<uint8_t>> MnemonicToEntropy(
    const std::string& mnemonic);
bool IsValidMnemonic(const std::string& mnemonic);

bool EncodeString(const std::string& input, std::string* output);
bool EncodeStringArray(const std::vector<std::string>& input,
                       std::string* output);

bool DecodeString(size_t offset, const std::string& input, std::string* output);

// Updates preferences for when the wallet is unlocked.
// This is done in a utils function instead of in the KeyringService
// because we call it both from the old extension and the new wallet when
// it unlocks.
void UpdateLastUnlockPref(PrefService* prefs);

// Use kBraveWalletLastUnlockTime pref to determine if any wallets has been
// created before, regardless of still existed or not.
bool HasCreatedWallets(PrefService* prefs);

base::Value::Dict TransactionReceiptToValue(
    const TransactionReceipt& tx_receipt);
std::optional<TransactionReceipt> ValueToTransactionReceipt(
    const base::Value::Dict& value);

std::vector<mojom::NetworkInfoPtr> GetAllKnownChains(PrefService* prefs,
                                                     mojom::CoinType coin);
std::vector<mojom::NetworkInfoPtr> GetAllCustomChains(PrefService* prefs,
                                                      mojom::CoinType coin);
std::vector<mojom::NetworkInfoPtr> GetAllChains(PrefService* prefs,
                                                mojom::CoinType coin);
mojom::NetworkInfoPtr GetKnownChain(PrefService* prefs,
                                    const std::string& chain_id,
                                    mojom::CoinType coin);
mojom::NetworkInfoPtr GetCustomChain(PrefService* prefs,
                                     const std::string& chain_id,
                                     mojom::CoinType coin);
mojom::NetworkInfoPtr GetChain(PrefService* prefs,
                               const std::string& chain_id,
                               mojom::CoinType coin);
bool KnownChainExists(const std::string& chain_id, mojom::CoinType coin);
bool CustomChainExists(PrefService* prefs,
                       const std::string& custom_chain_id,
                       mojom::CoinType coin);
std::vector<std::string> CustomChainsExist(
    PrefService* prefs,
    const std::vector<std::string>& custom_chain_ids,
    mojom::CoinType coin);

GURL GetNetworkURL(PrefService* prefs,
                   const std::string& chain_id,
                   mojom::CoinType coin);
GURL GetInfuraURLForKnownChainId(const std::string& chain_id);
std::string GetInfuraEndpointForKnownChainId(const std::string& chain_id);
std::string GetInfuraSubdomainForKnownChainId(const std::string& chain_id);
GURL AddInfuraProjectId(const GURL& url);
GURL MaybeAddInfuraProjectId(const GURL& url);

std::string GetSolanaSubdomainForKnownChainId(const std::string& chain_id);
std::string GetFilecoinSubdomainForKnownChainId(const std::string& chain_id);
std::string GetBitcoinSubdomainForKnownChainId(const std::string& chain_id);
std::string GetKnownFilNetworkId(const std::string& chain_id);
std::string GetKnownBtcNetworkId(const std::string& chain_id);
std::string GetKnownSolNetworkId(const std::string& chain_id);
std::string GetKnownZecNetworkId(const std::string& chain_id);
std::string GetKnownNetworkId(mojom::CoinType coin,
                              const std::string& chain_id);
std::string GetNetworkId(PrefService* prefs,
                         mojom::CoinType coin,
                         const std::string& chain_id);
void SetDefaultEthereumWallet(PrefService* prefs,
                              mojom::DefaultWallet default_wallet);
void SetDefaultSolanaWallet(PrefService* prefs,
                            mojom::DefaultWallet default_wallet);
mojom::DefaultWallet GetDefaultEthereumWallet(PrefService* prefs);
mojom::DefaultWallet GetDefaultSolanaWallet(PrefService* prefs);
void SetDefaultBaseCurrency(PrefService* prefs, const std::string& currency);
std::string GetDefaultBaseCurrency(PrefService* prefs);
void SetDefaultBaseCryptocurrency(PrefService* prefs,
                                  const std::string& cryptocurrency);
std::string GetDefaultBaseCryptocurrency(PrefService* prefs);
std::vector<std::string> GetAllKnownEthNetworkIds();
std::vector<std::string> GetAllKnownSolNetworkIds();
std::vector<std::string> GetAllKnownFilNetworkIds();
std::vector<std::string> GetAllKnownBtcNetworkIds();
std::vector<std::string> GetAllKnownZecNetworkIds();
std::string GetKnownEthNetworkId(const std::string& chain_id);

GURL GetUnstoppableDomainsRpcUrl(const std::string& chain_id);
std::string GetUnstoppableDomainsProxyReaderContractAddress(
    const std::string& chain_id);
GURL GetEnsRpcUrl();
std::string GetEnsRegistryContractAddress(const std::string& chain_id);
GURL GetSnsRpcUrl();

// Append chain value to kBraveWalletCustomNetworks dictionary pref.
void AddCustomNetwork(PrefService* prefs, const mojom::NetworkInfo& chain);

void RemoveCustomNetwork(PrefService* prefs,
                         const std::string& chain_id_to_remove,
                         mojom::CoinType coin);

std::vector<std::string> GetHiddenNetworks(PrefService* prefs,
                                           mojom::CoinType coin);
void AddHiddenNetwork(PrefService* prefs,
                      mojom::CoinType coin,
                      const std::string& chain_id);
void RemoveHiddenNetwork(PrefService* prefs,
                         mojom::CoinType coin,
                         const std::string& chain_id);

// Get/Set the current chain ID for coin from kBraveWalletSelectedNetworks pref
// when origin is not presetned. If origin is presented,
// kBraveWalletSelectedNetworksPerOrigin will be used. In addition, if origin is
// opaque, we will also fallback to kBraveWalletSelectedNetworks but it will be
// read only, other non http/https scheme will fallback to r/w
// kBraveWalletSelectedNetworks.
std::string GetCurrentChainId(PrefService* prefs,
                              mojom::CoinType coin,
                              const std::optional<url::Origin>& origin);
bool SetCurrentChainId(PrefService* prefs,
                       mojom::CoinType coin,
                       const std::optional<url::Origin>& origin,
                       const std::string& chain_id);

std::string GetPrefKeyForCoinType(mojom::CoinType coin);

// Converts string representation of CoinType to enum.
std::optional<mojom::CoinType> GetCoinTypeFromPrefKey(const std::string& key);

// Resolves chain_id from network_id.
std::optional<std::string> GetChainId(PrefService* prefs,
                                      const mojom::CoinType& coin,
                                      const std::string& network_id);

// Resolves chain_id from network_id (including custom networks).
std::optional<std::string> GetChainIdByNetworkId(PrefService* prefs,
                                                 const mojom::CoinType& coin,
                                                 const std::string& network_id);

// Returns a string used for web3_clientVersion in the form of
// BraveWallet/v[chromium-version]. Note that we expose only the Chromium
// version and not the Brave version because that way no extra entropy
// is leaked from what the user agent provides for fingerprinting.
std::string GetWeb3ClientVersion();

// Given an url, return eTLD + 1 for that Origin
std::string eTLDPlusOne(const url::Origin& origin);

mojom::OriginInfoPtr MakeOriginInfo(const url::Origin& origin);

// Hex string of random 32 bytes.
std::string GenerateRandomHexString();

std::string WalletInternalErrorMessage();

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_UTILS_H_
