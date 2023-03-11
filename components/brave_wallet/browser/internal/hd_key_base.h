/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_BASE_H_

#include <memory>
#include <string>
#include <vector>

namespace brave_wallet {

constexpr char kMasterNode[] = "m";

class HDKeyBase {
 public:
  virtual ~HDKeyBase() = default;

  // Returns derivation path for this key if dervived from master key. Returns
  // empty string for all sorts of imported keys.
  virtual std::string GetPath() const = 0;

  // index should be 0 to 2^31-1
  // If anything failed, nullptr will be returned
  virtual std::unique_ptr<HDKeyBase> DeriveNormalChild(uint32_t index) = 0;

  // index should be 0 to 2^31-1
  // If anything failed, nullptr will be returned
  virtual std::unique_ptr<HDKeyBase> DeriveHardenedChild(uint32_t index) = 0;

  // path format: m/[n|n']*/[n|n']*...
  // n: 0 to 2^31-1 (normal derivation)
  // n': n + 2^31 (harden derivation)
  // If path is invalid, nullptr will be returned
  virtual std::unique_ptr<HDKeyBase> DeriveChildFromPath(
      const std::string& path) = 0;

  virtual std::vector<uint8_t> Sign(const std::vector<uint8_t>& msg,
                                    int* recid) = 0;
  virtual bool Verify(const std::vector<uint8_t>& msg,
                      const std::vector<uint8_t>& sig) = 0;

  virtual std::string EncodePrivateKeyForExport() const = 0;
  virtual std::vector<uint8_t> GetPrivateKeyBytes() const = 0;
  virtual std::vector<uint8_t> GetPublicKeyBytes() const = 0;
};
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_BASE_H_
