/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/device_id/device_id_impl.h"

#include <array>

#include "base/ranges/algorithm.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_ads {

namespace {

struct DisallwedMask {
  template <typename... Args>
  constexpr DisallwedMask(Args... args)  // NOLINT(runtime/explicit)
      : address{static_cast<uint8_t>(args)...}, size(sizeof...(args)) {}

  base::span<const uint8_t> as_span() const {
    return base::span(address).first(size);
  }

  std::array<uint8_t, 6u> address;
  size_t size;
};

void TestDisallowedRange(DisallwedMask mask) {
  std::array<uint8_t, 6u> address;

  // test the lower bound of the match
  base::ranges::fill(address, 0u);
  base::as_writable_byte_span(address).first(mask.size).copy_from(
      mask.as_span());
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(address));

  // test the upper bound of the match
  base::ranges::fill(
      base::as_writable_byte_span(address).split_at(mask.size).second, 0xffu);
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(address));

  // test accepted just outside the upper bound
  base::as_writable_byte_span(address).first(mask.size).copy_from(
      mask.as_span());
  base::as_writable_byte_span(address).first(mask.size).last<1>()[0] += 0x01;
  base::ranges::fill(
      base::as_writable_byte_span(address).split_at(mask.size).second, 0x00u);
  EXPECT_TRUE(DeviceIdImpl::IsValidMacAddress(address));

  // test accepted just outside the lower bound
  base::as_writable_byte_span(address).first(mask.size).copy_from(
      mask.as_span());
  base::as_writable_byte_span(address).first(mask.size).last<1>()[0] -= 0x01;
  base::ranges::fill(
      base::as_writable_byte_span(address).split_at(mask.size).second, 0xffu);
  EXPECT_TRUE(DeviceIdImpl::IsValidMacAddress(address));
}

}  // namespace

TEST(DeviceIdImplTest, InvalidMacAddresses) {
  // VMware
  TestDisallowedRange({0x00, 0x50, 0x56});
  TestDisallowedRange({0x00, 0x05, 0x69});
  TestDisallowedRange({0x00, 0x0c, 0x29});
  TestDisallowedRange({0x00, 0x1c, 0x14});
  // VirtualBox
  TestDisallowedRange({0x08, 0x00, 0x27});
  // Sierra Wireless cell modems.
  TestDisallowedRange({0x00, 0xA0, 0xD5});
  // Fortinet VPN
  TestDisallowedRange({0x00, 0x09, 0x0F});

  // Juniper
  EXPECT_TRUE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0xfe, 0xff, 0xff, 0xff, 0xff})));
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0xff, 0x00, 0x00, 0x00, 0x00})));
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0xff, 0xff, 0xff, 0xff, 0xff})));
  EXPECT_TRUE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x01, 0x00, 0x00, 0x00, 0x00, 0x00})));

  // T-Mobile Wireless Ethernet
  // Qualcomm USB ethernet adapter
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0xa0, 0xc6, 0x00, 0x00, 0x00})));
  // Windows VPN
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x53, 0x45, 0x00, 0x00, 0x00})));
  // Bluetooth
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x1f, 0x81, 0x00, 0x08, 0x30})));
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x1b, 0x10, 0x00, 0x2a, 0xec})));
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x15, 0x83, 0x15, 0xa3, 0x10})));
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x15, 0x83, 0x07, 0xC6, 0x5A})));
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x1f, 0x81, 0x00, 0x02, 0x00})));
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x1f, 0x81, 0x00, 0x02, 0xdd})));
  // Ceton TV tuner
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x22, 0x2c, 0xff, 0xff, 0xff})));
  // Check Point VPN
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x54, 0x55, 0x43, 0x44, 0x52, 0x09})));
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x54, 0xEF, 0x14, 0x71, 0xE4, 0x0E})));
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x54, 0xBA, 0xC6, 0xFF, 0x74, 0x10})));
  // Cisco VPN
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x05, 0x9a, 0x3c, 0x78, 0x00})));
  // Intel USB cell modem
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x1e, 0x10, 0x1f, 0x00, 0x01})));
  // Microsoft tethering
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x80, 0x00, 0x60, 0x0f, 0xe8, 0x00})));
  // Nortel VPN
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x44, 0x45, 0x53, 0x54, 0x42, 0x00})));
  // AEP VPN
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x30, 0x70, 0x00, 0x00, 0x01})));
  // Positive VPN
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x02, 0x03, 0x04, 0x05, 0x06})));
  // Bluetooth
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x15, 0x83, 0x0B, 0x13, 0xC0})));

  // unknown by last byte form the last test
  EXPECT_TRUE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x00, 0x15, 0x83, 0x0B, 0x13, 0xC1})));

  // Fails because range is too short
  EXPECT_FALSE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x01, 0x14, 0x85})));
  EXPECT_TRUE(DeviceIdImpl::IsValidMacAddress(
      std::to_array<uint8_t>({0x01, 0x14, 0x85, 0x01, 0x14, 0x85})));
}

}  // namespace brave_ads
