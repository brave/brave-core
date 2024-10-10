/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/device_id/device_id_impl.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/hmac.h"

namespace brave_ads {

namespace {

std::optional<std::string> ComputeHmacSha256(std::string_view key,
                                             std::string_view text) {
  crypto::HMAC hmac(crypto::HMAC::SHA256);
  std::array<uint8_t, 32u> digest;
  if (!hmac.Init(key) ||
      !hmac.Sign(base::as_bytes(base::make_span(text)), digest)) {
    return std::nullopt;
  }

  return base::ToLowerASCII(base::HexEncode(digest));
}

void GetRawDeviceIdCallback(DeviceIdCallback callback,
                            std::string raw_device_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (raw_device_id.empty()) {
    return std::move(callback).Run({});
  }

  std::move(callback).Run(
      ComputeHmacSha256(raw_device_id, "FOOBAR").value_or(std::string()));
}

bool IsValidMacAddressImpl(const void* bytes, size_t size) {
  constexpr size_t kMacLength = 6;

  struct MacAddressInfo {
    size_t count;
    unsigned char address[kMacLength];
  };

  constexpr size_t kOuiLength = 3;

  // VPN, virtualization, tethering, bluetooth, etc.
  static constexpr MacAddressInfo kInvalidMacAddresses[] = {
      // Empty address
      {kMacLength, {0, 0, 0, 0, 0, 0}},
      // VMware
      {kOuiLength, {0x00, 0x50, 0x56}},
      {kOuiLength, {0x00, 0x05, 0x69}},
      {kOuiLength, {0x00, 0x0c, 0x29}},
      {kOuiLength, {0x00, 0x1c, 0x14}},
      // VirtualBox
      {kOuiLength, {0x08, 0x00, 0x27}},
      // PdaNet
      {kMacLength, {0x00, 0x26, 0x37, 0xbd, 0x39, 0x42}},
      // Cisco AnyConnect VPN
      {kMacLength, {0x00, 0x05, 0x9a, 0x3c, 0x7a, 0x00}},
      // Marvell sometimes uses this as a dummy address
      {kMacLength, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}},
      // Apple uses this across machines for Bluetooth ethernet adapters.
      {kMacLength - 1, {0x65, 0x90, 0x07, 0x42, 0xf1}},
      // Juniper uses this for their Virtual Adapter, the other 4 bytes are
      // reassigned at every boot. 00-ff-xx is not assigned to anyone.
      {2, {0x00, 0xff}},
      // T-Mobile Wireless Ethernet
      {kMacLength, {0x00, 0xa0, 0xc6, 0x00, 0x00, 0x00}},
      // Generic Bluetooth device
      {kMacLength, {0x00, 0x15, 0x83, 0x3d, 0x0a, 0x57}},
      // RAS Async Adapter
      {kMacLength, {0x20, 0x41, 0x53, 0x59, 0x4e, 0xff}},
      // Qualcomm USB ethernet adapter
      {kMacLength, {0x00, 0xa0, 0xc6, 0x00, 0x00, 0x00}},
      // Windows VPN
      {kMacLength, {0x00, 0x53, 0x45, 0x00, 0x00, 0x00}},
      // Bluetooth
      {kMacLength, {0x00, 0x1f, 0x81, 0x00, 0x08, 0x30}},
      {kMacLength, {0x00, 0x1b, 0x10, 0x00, 0x2a, 0xec}},
      {kMacLength, {0x00, 0x15, 0x83, 0x15, 0xa3, 0x10}},
      {kMacLength, {0x00, 0x15, 0x83, 0x07, 0xC6, 0x5A}},
      {kMacLength, {0x00, 0x1f, 0x81, 0x00, 0x02, 0x00}},
      {kMacLength, {0x00, 0x1f, 0x81, 0x00, 0x02, 0xdd}},
      // Ceton TV tuner
      {kMacLength, {0x00, 0x22, 0x2c, 0xff, 0xff, 0xff}},
      // Check Point VPN
      {kMacLength, {0x54, 0x55, 0x43, 0x44, 0x52, 0x09}},
      {kMacLength, {0x54, 0xEF, 0x14, 0x71, 0xE4, 0x0E}},
      {kMacLength, {0x54, 0xBA, 0xC6, 0xFF, 0x74, 0x10}},
      // Cisco VPN
      {kMacLength, {0x00, 0x05, 0x9a, 0x3c, 0x7a, 0x00}},
      // Cisco VPN
      {kMacLength, {0x00, 0x05, 0x9a, 0x3c, 0x78, 0x00}},
      // Intel USB cell modem
      {kMacLength, {0x00, 0x1e, 0x10, 0x1f, 0x00, 0x01}},
      // Microsoft tethering
      {kMacLength, {0x80, 0x00, 0x60, 0x0f, 0xe8, 0x00}},
      // Nortel VPN
      {kMacLength, {0x44, 0x45, 0x53, 0x54, 0x42, 0x00}},
      // AEP VPN
      {kMacLength, {0x00, 0x30, 0x70, 0x00, 0x00, 0x01}},
      // Positive VPN
      {kMacLength, {0x00, 0x02, 0x03, 0x04, 0x05, 0x06}},
      // Bluetooth
      {kMacLength, {0x00, 0x15, 0x83, 0x0B, 0x13, 0xC0}},
      // Kerio Virtual Network Adapter
      {kMacLength, {0x44, 0x45, 0x53, 0x54, 0x4f, 0x53}},
      // Sierra Wireless cell modems.
      {kOuiLength, {0x00, 0xA0, 0xD5}},
      // FRITZ!web DSL
      {kMacLength, {0x00, 0x04, 0x0E, 0xFF, 0xFF, 0xFF}},
      // VirtualPC
      {kMacLength, {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}},
      // Bluetooth
      {kMacLength, {0x00, 0x1F, 0x81, 0x00, 0x01, 0x00}},
      {kMacLength, {0x00, 0x30, 0x91, 0x10, 0x00, 0x26}},
      {kMacLength, {0x00, 0x25, 0x00, 0x5A, 0xC3, 0xD0}},
      {kMacLength, {0x00, 0x15, 0x83, 0x0C, 0xBF, 0xEB}},
      // Huawei cell modem
      {kMacLength, {0x58, 0x2C, 0x80, 0x13, 0x92, 0x63}},
      // Fortinet VPN
      {kOuiLength, {0x00, 0x09, 0x0F}},
      // Realtek
      {kMacLength, {0x00, 0x00, 0x00, 0x00, 0x00, 0x30}},
      // Other rare dupes.
      {kMacLength, {0x00, 0x11, 0xf5, 0x0d, 0x8a, 0xe8}},  // Atheros
      {kMacLength, {0x00, 0x20, 0x07, 0x01, 0x16, 0x06}},  // Atheros
      {kMacLength, {0x0d, 0x0b, 0x00, 0x00, 0xe0, 0x00}},  // Atheros
      {kMacLength, {0x90, 0x4c, 0xe5, 0x0b, 0xc8, 0x8e}},  // Atheros
      {kMacLength, {0x00, 0x1c, 0x23, 0x38, 0x49, 0xa4}},  // Broadcom
      {kMacLength, {0x00, 0x12, 0x3f, 0x82, 0x7c, 0x32}},  // Broadcom
      {kMacLength, {0x00, 0x11, 0x11, 0x32, 0xc3, 0x77}},  // Broadcom
      {kMacLength, {0x00, 0x24, 0xd6, 0xae, 0x3e, 0x39}},  // Microsoft
      {kMacLength, {0x00, 0x0f, 0xb0, 0x3a, 0xb4, 0x80}},  // Realtek
      {kMacLength, {0x08, 0x10, 0x74, 0xa1, 0xda, 0x1b}},  // Realtek
      {kMacLength, {0x00, 0x21, 0x9b, 0x2a, 0x0a, 0x9c}},  // Realtek
  };

  if (size != kMacLength) {
    return false;
  }

  if (static_cast<const unsigned char*>(bytes)[0] & 0x02) {
    // Locally administered.
    return false;
  }

  for (const auto& mac_address : kInvalidMacAddresses) {
    if (memcmp(mac_address.address, bytes, mac_address.count) == 0) {
      return false;
    }
  }

  return true;
}

}  // namespace

void DeviceIdImpl::GetDeviceId(DeviceIdCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Forward call to platform specific implementation, then compute the HMAC
  // in the callback.
  GetRawDeviceId(base::BindOnce(&GetRawDeviceIdCallback, std::move(callback)));
}

// static
bool DeviceIdImpl::IsValidMacAddress(const void* bytes, size_t size) {
  return IsValidMacAddressImpl(bytes, size);
}

}  // namespace brave_ads
