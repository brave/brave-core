/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/device_id/device_id_impl.h"

#include <CoreFoundation/CoreFoundation.h>
#include <DiskArbitration/DADisk.h>
#include <DiskArbitration/DASession.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkInterface.h>
#include <sys/mount.h>

#include <string>
#include <utility>

#include "base/apple/foundation_util.h"
#include "base/apple/scoped_cftyperef.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_ioobject.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/threading/scoped_blocking_call.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace brave_ads {

using IsValidMacAddressCallback =
    base::RepeatingCallback<bool(base::span<const uint8_t> bytes)>;

namespace {

constexpr char kRootDirectory[] = "/";

// Return the BSD name (e.g. '/dev/disk1') of the root directory by enumerating
// through the mounted volumes. Returns an empty string if an error occurred.
std::string FindBSDNameOfSystemDisk() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  struct statfs* mounted_volumes;
  const int count = getmntinfo_r_np(&mounted_volumes, 0);
  if (count == 0) {
    return {};
  }

  std::string root_bsd_name;
  for (int i = 0; i < count; i++) {
    const struct statfs& volume = UNSAFE_TODO(mounted_volumes[i]);
    if (std::string(volume.f_mntonname) == kRootDirectory) {
      root_bsd_name = std::string(volume.f_mntfromname);
      break;
    }
  }

  free(mounted_volumes);
  return root_bsd_name;
}

// Return the Volume UUID property of a BSD disk name (e.g. '/dev/disk1').
// Returns an empty string if an error occurred.
std::string GetVolumeUUIDFromBSDName(const std::string& bsd_name) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  const CFAllocatorRef allocator = nullptr;

  const base::apple::ScopedCFTypeRef<DASessionRef> session(
      DASessionCreate(allocator));
  if (!session) {
    return {};
  }

  const base::apple::ScopedCFTypeRef<DADiskRef> disk(
      DADiskCreateFromBSDName(allocator, session.get(), bsd_name.c_str()));
  if (!disk) {
    return {};
  }

  const base::apple::ScopedCFTypeRef<CFDictionaryRef> disk_description(
      DADiskCopyDescription(disk.get()));
  if (!disk_description) {
    return {};
  }

  const CFUUIDRef volume_uuid = base::apple::GetValueFromDictionary<CFUUIDRef>(
      disk_description.get(), kDADiskDescriptionVolumeUUIDKey);
  if (volume_uuid == nullptr) {
    return {};
  }

  const base::apple::ScopedCFTypeRef<CFStringRef> volume_uuid_as_string(
      CFUUIDCreateString(allocator, volume_uuid));
  if (!volume_uuid_as_string) {
    return {};
  }

  return base::SysCFStringRefToUTF8(volume_uuid_as_string.get());
}

std::string GetSystemVolumeUUID() {
  std::string result;

  const std::string bsd_name = FindBSDNameOfSystemDisk();
  if (!bsd_name.empty()) {
    result = GetVolumeUUIDFromBSDName(bsd_name);
  }

  return result;
}

class MacAddressProcessor {
 public:
  explicit MacAddressProcessor(
      IsValidMacAddressCallback is_valid_mac_address_callback)
      : is_valid_mac_address_callback_(
            std::move(is_valid_mac_address_callback)) {}

  bool ProcessNetworkController(io_object_t network_controller) {
    bool keep_going = true;

    const base::apple::ScopedCFTypeRef<CFDataRef> mac_address_data(
        static_cast<CFDataRef>(IORegistryEntryCreateCFProperty(
            network_controller, CFSTR(kIOMACAddress), kCFAllocatorDefault, 0)));
    if (!mac_address_data) {
      return keep_going;
    }

    auto mac_address_bytes = base::as_bytes(UNSAFE_TODO(base::make_span(
        CFDataGetBytePtr(mac_address_data.get()),
        base::checked_cast<size_t>(CFDataGetLength(mac_address_data.get())))));
    if (!is_valid_mac_address_callback_.Run(mac_address_bytes)) {
      return keep_going;
    }

    mac_address_ = base::ToLowerASCII(base::HexEncode(mac_address_bytes));

    base::apple::ScopedCFTypeRef<CFStringRef> provider_class_string(
        static_cast<CFStringRef>(IORegistryEntryCreateCFProperty(
            network_controller, CFSTR(kIOProviderClassKey), kCFAllocatorDefault,
            0)));
    if (provider_class_string) {
      if (CFStringCompare(provider_class_string.get(), CFSTR("IOPCIDevice"),
                          0) == kCFCompareEqualTo) {
        // MAC address from built-in network card is always the best choice.
        keep_going = false;
      }
    }

    return keep_going;
  }

  std::string GetMacAddress() const { return mac_address_; }

 private:
  IsValidMacAddressCallback is_valid_mac_address_callback_;
  std::string mac_address_;
};

std::string GetMacAddress(
    IsValidMacAddressCallback is_valid_mac_address_callback) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  mach_port_t main_port;
  kern_return_t result = IOMasterPort(MACH_PORT_NULL, &main_port);
  if (result != KERN_SUCCESS) {
    return {};
  }

  CFMutableDictionaryRef matching =
      IOServiceMatching(kIOEthernetInterfaceClass);
  if (!matching) {
    return {};
  }

  io_iterator_t iterator;
  result = IOServiceGetMatchingServices(main_port, matching, &iterator);
  if (result != KERN_SUCCESS) {
    return {};
  }
  base::mac::ScopedIOObject<io_iterator_t> scoped_iterator(iterator);

  MacAddressProcessor processor(std::move(is_valid_mac_address_callback));
  while (true) {
    // NOTE: `service` should not be released.
    const io_object_t service = IOIteratorNext(scoped_iterator.get());
    if (!service) {
      break;
    }

    io_object_t parent;
    result = IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent);
    if (result == KERN_SUCCESS) {
      const base::mac::ScopedIOObject<io_object_t> scoped_parent(parent);
      if (!processor.ProcessNetworkController(scoped_parent.get())) {
        break;
      }
    }
  }

  return processor.GetMacAddress();
}

void GetRawDeviceIdImpl(IsValidMacAddressCallback is_valid_mac_address_callback,
                        DeviceIdCallback callback) {
  const std::string mac_address =
      GetMacAddress(std::move(is_valid_mac_address_callback));

  const std::string system_volume_uuid = GetSystemVolumeUUID();

  const std::string platform_serial_number =
      base::mac::GetPlatformSerialNumber();

  std::string raw_device_id;
  if (!system_volume_uuid.empty() && !platform_serial_number.empty()) {
    raw_device_id =
        base::StrCat({mac_address, system_volume_uuid, platform_serial_number});
  }

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(raw_device_id)));
}

}  // namespace

// static
void DeviceIdImpl::GetRawDeviceId(DeviceIdCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&GetRawDeviceIdImpl,
                     base::BindRepeating(&DeviceIdImpl::IsValidMacAddress),
                     std::move(callback)));
}

}  // namespace brave_ads
