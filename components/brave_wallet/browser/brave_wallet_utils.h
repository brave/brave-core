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

// When we call memset in end of function to clean local variables
// for security reason, compiler optimizer can remove such call.
// So we use our own function for this purpose.
void SecureZeroData(void* data, size_t size);

// Updates preferences for when the wallet is unlocked.
// This is done in a utils function instead of in the KeyringController
// because we call it both from the old extension and the new wallet when
// it unlocks.
void UpdateLastUnlockPref(PrefService* prefs);

base::Value TransactionReceiptToValue(const TransactionReceipt& tx_receipt);
absl::optional<TransactionReceipt> ValueToTransactionReceipt(
    const base::Value& value);

void GetAllKnownChains(PrefService* prefs,
                       std::vector<mojom::EthereumChainPtr>* chains);
const std::vector<mojom::EthereumChain> GetAllKnownNetworksForTesting();
void GetAllCustomChains(PrefService* prefs,
                        std::vector<mojom::EthereumChainPtr>* result);
void GetAllChains(PrefService* prefs,
                  std::vector<mojom::EthereumChainPtr>* result);
GURL GetNetworkURL(PrefService* prefs, const std::string& chain_id);
std::string GetInfuraSubdomainForKnownChainId(const std::string& chain_id);
mojom::EthereumChainPtr GetKnownChain(PrefService* prefs,
                                      const std::string& chain_id);
std::string GetNetworkId(PrefService* prefs, const std::string& chain_id);
void SetDefaultWallet(PrefService* prefs, mojom::DefaultWallet default_wallet);
mojom::DefaultWallet GetDefaultWallet(PrefService* prefs);
void SetDefaultBaseCurrency(PrefService* prefs, const std::string& currency);
std::string GetDefaultBaseCurrency(PrefService* prefs);
void SetDefaultBaseCryptocurrency(PrefService* prefs,
                                  const std::string& cryptocurrency);
std::string GetDefaultBaseCryptocurrency(PrefService* prefs);
std::vector<std::string> GetAllKnownNetworkIds();
std::string GetKnownNetworkId(const std::string& chain_id);

std::string GetUnstoppableDomainsProxyReaderContractAddress(
    const std::string& chain_id);
std::string GetEnsRegistryContractAddress(const std::string& chain_id);

// Append chain value to kBraveWalletCustomNetworks list pref.
void AddCustomNetwork(PrefService* prefs, mojom::EthereumChainPtr chain);

// Get a specific chain from all chains.
mojom::EthereumChainPtr GetChain(PrefService* prefs,
                                 const std::string& chain_id);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_UTILS_H_
