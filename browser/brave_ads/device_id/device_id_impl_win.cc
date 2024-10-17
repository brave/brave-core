/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/device_id/device_id_impl.h"

// Note: The order of header includes is important, as we want both pre-Vista
// and post-Vista data structures to be defined, specifically
// PIP_ADAPTER_ADDRESSES and PMIB_IF_ROW2.

// clang-format off
// NOLINTBEGIN(sort-order)
#include <limits.h>
#include <winsock2.h>
#include <ws2def.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
// NOLINTEND(sort-order)
// clang-format on

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/scoped_native_library.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/win/windows_version.h"
#include "components/metrics/machine_id_provider.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace brave_ads {

namespace {

using IsValidMacAddressCallback =
    base::RepeatingCallback<bool(base::span<const uint8_t> bytes)>;

class MacAddressProcessor {
 public:
  explicit MacAddressProcessor(
      IsValidMacAddressCallback is_valid_mac_address_callback)
      : is_valid_mac_address_callback_(
            std::move(is_valid_mac_address_callback)),
        found_index_(ULONG_MAX) {}

  // Iterate through the interfaces, looking for a valid MAC address with the
  // lowest IfIndex.
  void ProcessAdapterAddress(PIP_ADAPTER_ADDRESSES address) {
    if (address->IfType == IF_TYPE_TUNNEL) {
      return;
    }

    ProcessPhysicalAddress(address->IfIndex, address->PhysicalAddress,
                           address->PhysicalAddressLength);
  }

  void ProcessInterfaceRow(const PMIB_IF_ROW2 row) {
    if (row->Type == IF_TYPE_TUNNEL ||
        !row->InterfaceAndOperStatusFlags.HardwareInterface) {
      return;
    }

    ProcessPhysicalAddress(row->InterfaceIndex, row->PhysicalAddress,
                           row->PhysicalAddressLength);
  }

  std::string GetMacAddress() const { return mac_address_; }

 private:
  void ProcessPhysicalAddress(NET_IFINDEX index,
                              const void* bytes,
                              size_t size) {
    if (index >= found_index_ || size == 0)
      return;

    auto mac_address_bytes =
        UNSAFE_TODO(base::span(static_cast<const uint8_t*>(bytes), size));
    if (!is_valid_mac_address_callback_.Run(mac_address_bytes)) {
      return;
    }

    mac_address_ = base::ToLowerASCII(base::HexEncode(mac_address_bytes));

    found_index_ = index;
  }

  IsValidMacAddressCallback is_valid_mac_address_callback_;
  std::string mac_address_;
  NET_IFINDEX found_index_;
};

std::string GetMacAddressFromGetAdaptersAddresses(
    IsValidMacAddressCallback is_valid_mac_address_callback) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  // Microsoft recommend a default size of 15k.
  ULONG buffer_size = 15 * 1024;

  // Disable as much as we can, since all we want is MAC addresses.
  const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER |
                      GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_MULTICAST |
                      GAA_FLAG_SKIP_UNICAST;

  std::vector<unsigned char> buffer(buffer_size);
  PIP_ADAPTER_ADDRESSES adapter_addresses =
      reinterpret_cast<PIP_ADAPTER_ADDRESSES>(&buffer.front());

  DWORD result = GetAdaptersAddresses(AF_UNSPEC, flags, 0, adapter_addresses,
                                      &buffer_size);
  if (result == ERROR_BUFFER_OVERFLOW) {
    buffer.resize(buffer_size);
    adapter_addresses =
        reinterpret_cast<PIP_ADAPTER_ADDRESSES>(&buffer.front());
    result = GetAdaptersAddresses(AF_UNSPEC, flags, 0, adapter_addresses,
                                  &buffer_size);
  }

  if (result != NO_ERROR) {
    return {};
  }

  MacAddressProcessor processor(std::move(is_valid_mac_address_callback));
  for (; adapter_addresses != NULL;
       adapter_addresses = adapter_addresses->Next) {
    processor.ProcessAdapterAddress(adapter_addresses);
  }

  return processor.GetMacAddress();
}

std::string GetMacAddressFromGetIfTable2(
    IsValidMacAddressCallback is_valid_mac_address_callback) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  // This is available on Vista+ only.
  base::ScopedNativeLibrary library(base::FilePath(L"Iphlpapi.dll"));

  typedef DWORD(NETIOAPI_API_ * GetIfTablePtr)(PMIB_IF_TABLE2*);
  typedef void(NETIOAPI_API_ * FreeMibTablePtr)(PMIB_IF_TABLE2);

  GetIfTablePtr get_if_table = reinterpret_cast<GetIfTablePtr>(
      library.GetFunctionPointer("GetIfTable2"));
  FreeMibTablePtr free_mib_table = reinterpret_cast<FreeMibTablePtr>(
      library.GetFunctionPointer("FreeMibTable"));
  if (get_if_table == NULL || free_mib_table == NULL) {
    return {};
  }

  PMIB_IF_TABLE2 if_table = NULL;
  const DWORD result = get_if_table(&if_table);
  if (result != NO_ERROR || if_table == NULL) {
    return {};
  }

  MacAddressProcessor processor(std::move(is_valid_mac_address_callback));
  for (size_t i = 0; i < if_table->NumEntries; i++) {
    processor.ProcessInterfaceRow(&(if_table->Table[i]));
  }

  if (if_table != NULL) {
    free_mib_table(if_table);
    if_table = NULL;
  }

  return processor.GetMacAddress();
}

void GetMacAddress(IsValidMacAddressCallback is_valid_mac_address_callback,
                   DeviceIdCallback callback) {
  std::string mac_address =
      GetMacAddressFromGetAdaptersAddresses(is_valid_mac_address_callback);
  if (mac_address.empty()) {
    mac_address = GetMacAddressFromGetIfTable2(is_valid_mac_address_callback);
  }

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), mac_address));
}

void GetMachineIdCallback(std::string mac_address,
                          DeviceIdCallback callback,
                          std::string machine_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::string raw_device_id;
  if (!machine_id.empty()) {
    raw_device_id = base::StrCat({mac_address, machine_id});
  }

  std::move(callback).Run(raw_device_id);
}

void GetMacAddressCallback(DeviceIdCallback callback, std::string mac_address) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&metrics::MachineIdProvider::GetMachineId),
      base::BindOnce(&GetMachineIdCallback, mac_address, std::move(callback)));
}

}  // namespace

// static
void DeviceIdImpl::GetRawDeviceId(DeviceIdCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(
          &GetMacAddress, base::BindRepeating(&DeviceIdImpl::IsValidMacAddress),
          base::BindOnce(&GetMacAddressCallback, std::move(callback))));
}

}  // namespace brave_ads
