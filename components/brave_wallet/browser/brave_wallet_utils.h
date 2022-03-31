/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_UTILS_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;
namespace base {
class Value;
}  // namespace base

class GURL;

namespace brave_wallet {

bool IsNativeWalletEnabled();
bool IsFilecoinEnabled();
bool IsSolanaEnabled();

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
bool DecodeStringArray(const std::string& input,
                       std::vector<std::string>* output);

// Updates preferences for when the wallet is unlocked.
// This is done in a utils function instead of in the KeyringService
// because we call it both from the old extension and the new wallet when
// it unlocks.
void UpdateLastUnlockPref(PrefService* prefs);

base::Value TransactionReceiptToValue(const TransactionReceipt& tx_receipt);
absl::optional<TransactionReceipt> ValueToTransactionReceipt(
    const base::Value& value);

void GetAllKnownEthChains(PrefService* prefs,
                          std::vector<mojom::NetworkInfoPtr>* chains);
const std::vector<mojom::NetworkInfoPtr> GetAllKnownNetworksForTesting();
void GetAllEthCustomChains(PrefService* prefs,
                           std::vector<mojom::NetworkInfoPtr>* result);
GURL GetFirstValidChainURL(const std::vector<std::string>& chain_urls);
void GetAllChains(PrefService* prefs,
                  mojom::CoinType coin,
                  std::vector<mojom::NetworkInfoPtr>* result);
GURL GetNetworkURL(PrefService* prefs,
                   const std::string& chain_id,
                   mojom::CoinType coin);
std::string GetInfuraSubdomainForKnownChainId(const std::string& chain_id);
mojom::NetworkInfoPtr GetKnownEthChain(PrefService* prefs,
                                       const std::string& chain_id);

std::string GetSolanaSubdomainForKnownChainId(const std::string& chain_id);
void GetAllKnownSolChains(std::vector<mojom::NetworkInfoPtr>* result);
std::string GetKnownSolNetworkId(const std::string& chain_id);
std::string GetKnownNetworkId(mojom::CoinType coin,
                              const std::string& chain_id);
std::string GetNetworkId(PrefService* prefs,
                         mojom::CoinType coin,
                         const std::string& chain_id);
void SetDefaultWallet(PrefService* prefs, mojom::DefaultWallet default_wallet);
mojom::DefaultWallet GetDefaultWallet(PrefService* prefs);
void SetDefaultBaseCurrency(PrefService* prefs, const std::string& currency);
std::string GetDefaultBaseCurrency(PrefService* prefs);
void SetDefaultBaseCryptocurrency(PrefService* prefs,
                                  const std::string& cryptocurrency);
std::string GetDefaultBaseCryptocurrency(PrefService* prefs);
std::vector<std::string> GetAllKnownEthNetworkIds();
std::string GetKnownEthNetworkId(const std::string& chain_id);

std::string GetUnstoppableDomainsProxyReaderContractAddress(
    const std::string& chain_id);
std::string GetEnsRegistryContractAddress(const std::string& chain_id);

// Append chain value to kBraveWalletCustomNetworks dictionary pref.
void AddCustomNetwork(PrefService* prefs, mojom::NetworkInfoPtr chain);

void RemoveCustomNetwork(PrefService* prefs,
                         const std::string& chain_id_to_remove);

// Get a specific chain from all chains for certain coin.
mojom::NetworkInfoPtr GetChain(PrefService* prefs,
                               const std::string& chain_id,
                               mojom::CoinType coin);

// Get the current chain ID for coin from kBraveWalletSelectedNetworks pref.
std::string GetCurrentChainId(PrefService* prefs, mojom::CoinType coin);

// Returns the first URL to use that:
// 1. Has no variables in it like ${INFURA_API_KEY}
// 2. Is HTTP or HTTPS
// Otherwise if there is a URL in the list, it returns the first one.
// Otherwise returns an empty GURL
GURL GetFirstValidChainURL(const std::vector<std::string>& chain_urls);

absl::optional<std::string> GetPrefKeyForCoinType(mojom::CoinType coin);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_UTILS_H_
