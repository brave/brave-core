/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/device_id/device_id_impl.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/types/zip.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/hmac.h"

namespace brave_ads {

namespace {

// A matcher for MAC addresses, to allows us to test against a mask of bytes.
class MacAddressInfoMatcher {
 public:
  template <typename... Args>
  constexpr MacAddressInfoMatcher(Args... args)  // NOLINT(runtime/explicit)
      : address_{static_cast<uint8_t>(args)...}, size_(sizeof...(args)) {
    base::ranges::fill(base::span(address_).split_at(size_).second, 0u);
    static_assert(sizeof...(args) <= 6u, "Invalid MAC address size");
  }

  constexpr MacAddressInfoMatcher(const MacAddressInfoMatcher&) = default;
  constexpr MacAddressInfoMatcher& operator=(
      const MacAddressInfoMatcher& other) {
    base::ranges::copy(other.address_, address_.begin());
    size_ = other.size_;
    return *this;
  }

  constexpr base::span<const uint8_t> mask() const {
    return base::span(address_).first(size_);
  }

 private:
  // The MAC address mask.
  std::array<uint8_t, 6u> address_;

  // The size of the partial mask.
  size_t size_;
};

// A comparator for MAC addresses, so it can be used with a flat_set.
struct MacAddressInfoComparator {
  using is_transparent = void;
  constexpr bool operator()(const MacAddressInfoMatcher& a,
                            const MacAddressInfoMatcher& b) const {
    return base::ranges::lexicographical_compare(a.mask(), b.mask());
  }
  constexpr bool operator()(const MacAddressInfoMatcher& matcher,
                            base::span<const uint8_t> address) const {
    return base::ranges::lexicographical_compare(
        matcher.mask(),
        address.first(std::min(matcher.mask().size(), address.size())));
  }
  constexpr bool operator()(base::span<const uint8_t> address,
                            const MacAddressInfoMatcher& matcher) const {
    return base::ranges::lexicographical_compare(
        address.first(std::min(matcher.mask().size(), address.size())),
        matcher.mask());
  }
};

constexpr auto kInvalidMacAddresses =
    base::MakeFixedFlatSet<MacAddressInfoMatcher>(
        {
            // Empty address
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
            // VMware
            {0x00, 0x50, 0x56},
            {0x00, 0x05, 0x69},
            {0x00, 0x0c, 0x29},
            {0x00, 0x1c, 0x14},
            // VirtualBox
            {0x08, 0x00, 0x27},
            // PdaNet
            {0x00, 0x26, 0x37, 0xbd, 0x39, 0x42},
            // Cisco AnyConnect VPN
            {0x00, 0x05, 0x9a, 0x3c, 0x7a, 0x00},
            // Marvell sometimes uses this as a dummy address
            {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
            // Apple uses this across machines for Bluetooth ethernet adapters.
            {0x65, 0x90, 0x07, 0x42, 0xf1},
            // Juniper uses this for their Virtual Adapter, the other 4 bytes
            // are
            // reassigned at every boot. 00-ff-xx is not assigned to anyone.
            {0x00, 0xff},
            // Generic Bluetooth device
            {0x00, 0x15, 0x83, 0x3d, 0x0a, 0x57},
            // RAS Async Adapter
            {0x20, 0x41, 0x53, 0x59, 0x4e, 0xff},
            // T-Mobile Wireless Ethernet
            // Qualcomm USB ethernet adapter
            {0x00, 0xa0, 0xc6, 0x00, 0x00, 0x00},
            // Windows VPN
            {0x00, 0x53, 0x45, 0x00, 0x00, 0x00},
            // Bluetooth
            {0x00, 0x1f, 0x81, 0x00, 0x08, 0x30},
            {0x00, 0x1b, 0x10, 0x00, 0x2a, 0xec},
            {0x00, 0x15, 0x83, 0x15, 0xa3, 0x10},
            {0x00, 0x15, 0x83, 0x07, 0xC6, 0x5A},
            {0x00, 0x1f, 0x81, 0x00, 0x02, 0x00},
            {0x00, 0x1f, 0x81, 0x00, 0x02, 0xdd},
            // Ceton TV tuner
            {0x00, 0x22, 0x2c, 0xff, 0xff, 0xff},
            // Check Point VPN
            {0x54, 0x55, 0x43, 0x44, 0x52, 0x09},
            {0x54, 0xEF, 0x14, 0x71, 0xE4, 0x0E},
            {0x54, 0xBA, 0xC6, 0xFF, 0x74, 0x10},
            // Cisco VPN
            {0x00, 0x05, 0x9a, 0x3c, 0x78, 0x00},
            // Intel USB cell modem
            {0x00, 0x1e, 0x10, 0x1f, 0x00, 0x01},
            // Microsoft tethering
            {0x80, 0x00, 0x60, 0x0f, 0xe8, 0x00},
            // Nortel VPN
            {0x44, 0x45, 0x53, 0x54, 0x42, 0x00},
            // AEP VPN
            {0x00, 0x30, 0x70, 0x00, 0x00, 0x01},
            // Positive VPN
            {0x00, 0x02, 0x03, 0x04, 0x05, 0x06},
            // Bluetooth
            {0x00, 0x15, 0x83, 0x0B, 0x13, 0xC0},
            // Kerio Virtual Network Adapter
            {0x44, 0x45, 0x53, 0x54, 0x4f, 0x53},
            // Sierra Wireless cell modems.
            {0x00, 0xA0, 0xD5},
            // FRITZ!web DSL
            {0x00, 0x04, 0x0E, 0xFF, 0xFF, 0xFF},
            // VirtualPC
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
            // Bluetooth
            {0x00, 0x1F, 0x81, 0x00, 0x01, 0x00},
            {0x00, 0x30, 0x91, 0x10, 0x00, 0x26},
            {0x00, 0x25, 0x00, 0x5A, 0xC3, 0xD0},
            {0x00, 0x15, 0x83, 0x0C, 0xBF, 0xEB},
            // Huawei cell modem
            {0x58, 0x2C, 0x80, 0x13, 0x92, 0x63},
            // Fortinet VPN
            {0x00, 0x09, 0x0F},
            // Realtek
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x30},
            // Other rare dupes.
            {0x00, 0x11, 0xf5, 0x0d, 0x8a, 0xe8},  // Atheros
            {0x00, 0x20, 0x07, 0x01, 0x16, 0x06},  // Atheros
            {0x0d, 0x0b, 0x00, 0x00, 0xe0, 0x00},  // Atheros
            {0x90, 0x4c, 0xe5, 0x0b, 0xc8, 0x8e},  // Atheros
            {0x00, 0x1c, 0x23, 0x38, 0x49, 0xa4},  // Broadcom
            {0x00, 0x12, 0x3f, 0x82, 0x7c, 0x32},  // Broadcom
            {0x00, 0x11, 0x11, 0x32, 0xc3, 0x77},  // Broadcom
            {0x00, 0x24, 0xd6, 0xae, 0x3e, 0x39},  // Microsoft
            {0x00, 0x0f, 0xb0, 0x3a, 0xb4, 0x80},  // Realtek
            {0x08, 0x10, 0x74, 0xa1, 0xda, 0x1b},  // Realtek
            {0x00, 0x21, 0x9b, 0x2a, 0x0a, 0x9c},  // Realtek
        },
        MacAddressInfoComparator());

// Ensure that invalid MAC addresses are not submasked. This is important as the
// binary search for the elements could fall past the bunch, if we had
// submasking groups, and then it wouldn't find the actual mask.
constexpr bool HasNoSubmaskingOnInvalidAddresses() {
  if constexpr (kInvalidMacAddresses.size() < 2u) {
    return true;
  }

  const auto next_in_range = base::span(kInvalidMacAddresses).subspan<1u>();
  for (const auto [it, next] : base::zip(kInvalidMacAddresses, next_in_range)) {
    if (it.mask() ==
        next.mask().first(std::min(it.mask().size(), next.mask().size()))) {
      return false;
    }
  }
  return true;
}
static_assert(HasNoSubmaskingOnInvalidAddresses(),
              "Invalid MAC addresses must not be submasked");

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

}  // namespace

void DeviceIdImpl::GetDeviceId(DeviceIdCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Forward call to platform specific implementation, then compute the HMAC
  // in the callback.
  GetRawDeviceId(base::BindOnce(&GetRawDeviceIdCallback, std::move(callback)));
}

// static
bool DeviceIdImpl::IsValidMacAddress(base::span<const uint8_t> bytes) {
  constexpr size_t kMacLength = 6;
  if (bytes.size() != kMacLength || bytes.front() & 0x02) {
    // Locally administered.
    return false;
  }

  return kInvalidMacAddresses.find(bytes) == kInvalidMacAddresses.end();
}

}  // namespace brave_ads
