/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_ECDH_AES_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_ECDH_AES_H_

#include <optional>
#include <string>
#include <vector>

namespace web_discovery {

struct AESEncryptResult {
  AESEncryptResult(std::vector<uint8_t> data,
                   std::string encoded_public_component_and_iv);
  ~AESEncryptResult();

  AESEncryptResult(const AESEncryptResult&);

  std::vector<uint8_t> data;
  std::string encoded_public_component_and_iv;
};

std::optional<AESEncryptResult> DeriveAESKeyAndEncrypt(
    const std::string& server_pub_key_b64,
    const std::vector<uint8_t>& data);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_ECDH_AES_H_
