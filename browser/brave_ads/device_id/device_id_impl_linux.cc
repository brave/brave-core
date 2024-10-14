/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/device_id/device_id_impl.h"

#include <sys/socket.h>

#include <ifaddrs.h>
#include <net/if.h>
#include <stddef.h>
#include <string.h>
#include <sys/ioctl.h>

#include <map>
#include <string>
#include <utility>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/threading/scoped_blocking_call.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace brave_ads {

using IsValidMacAddressCallback =
    base::RepeatingCallback<bool(base::span<const uint8_t> bytes)>;

using DiskMap = std::map<base::FilePath, base::FilePath>;

namespace {

constexpr char kDiskByUuidDirectoryName[] = "/dev/disk/by-uuid";
const char* const kDeviceNames[] = {"sda1", "hda1", "dm-0", "xvda1",
                                    "sda2", "hda2", "dm-1", "xvda2"};
const char* const kNetDeviceNamePrefixes[] = {
    // Fedora 15 uses biosdevname feature where Embedded ethernet uses the "em"
    // prefix and PCI cards use the p[0-9]c[0-9] format based on PCI slot and
    // card information.
    "eth", "em", "en", "wl", "ww", "p0", "p1", "p2",
    "p3",  "p4", "p5", "p6", "p7", "p8", "p9", "wlan"};

std::string GetDiskUuid() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  DiskMap disks;
  base::FileEnumerator files(base::FilePath(kDiskByUuidDirectoryName),
                             false,  // Recursive.
                             base::FileEnumerator::FILES);
  do {
    const base::FilePath file_path = files.Next();
    if (file_path.empty()) {
      break;
    }

    base::FilePath target_path;
    if (!base::ReadSymbolicLink(file_path, &target_path)) {
      continue;
    }

    const base::FilePath device_name = target_path.BaseName();
    const base::FilePath uuid = file_path.BaseName();
    disks[device_name] = uuid;
  } while (true);

  // Look for first device name matching an entry of `kDeviceNames`.
  std::string result;
  for (const char* device_name : kDeviceNames) {
    DiskMap::iterator iter = disks.find(base::FilePath(device_name));
    if (iter != disks.cend()) {
      result = iter->second.value();
      break;
    }
  }

  return result;
}

class MacAddressProcessor {
 public:
  explicit MacAddressProcessor(
      IsValidMacAddressCallback is_valid_mac_address_callback)
      : is_valid_mac_address_callback_(
            std::move(is_valid_mac_address_callback)) {}

  bool ProcessInterface(struct ifaddrs* ifaddr,
                        const char* const prefixes[],
                        size_t prefixes_count) {
    bool keep_going = true;


    struct ifreq ifinfo;
    memset(&ifinfo, 0, sizeof(ifinfo));
    strncpy(ifinfo.ifr_name, ifaddr->ifa_name, sizeof(ifinfo.ifr_name) - 1);

    const int sd = socket(AF_INET, SOCK_DGRAM, 0);
    const int result = ioctl(sd, SIOCGIFHWADDR, &ifinfo);
    close(sd);

    if (result != 0) {
      return keep_going;
    }

    constexpr size_t kMacLength = 6u;
    auto mac_address_bytes =
        base::as_bytes(base::make_span(ifinfo.ifr_hwaddr.sa_data, kMacLength));
    if (!is_valid_mac_address_callback_.Run(mac_address_bytes)) {
      return keep_going;
    }

    if (!IsValidPrefix(ifinfo.ifr_name, prefixes, prefixes_count)) {
      return keep_going;
    }

    keep_going = false;

    mac_address_ = base::ToLowerASCII(base::HexEncode(mac_address_bytes));

    return keep_going;
  }

  std::string GetMacAddress() const { return mac_address_; }

 private:
  bool IsValidPrefix(const char* name,
                     const char* const prefixes[],
                     size_t prefixes_count) {
    for (size_t i = 0; i < prefixes_count; i++) {
      if (strncmp(prefixes[i], name, strlen(prefixes[i])) == 0) {
        return true;
      }
    }

    return false;
  }

  IsValidMacAddressCallback is_valid_mac_address_callback_;
  std::string mac_address_;
};

std::string GetMacAddress(
    IsValidMacAddressCallback is_valid_mac_address_callback) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  struct ifaddrs* ifaddrs;
  const int result = getifaddrs(&ifaddrs);
  if (result < 0) {
    return {};
  }

  MacAddressProcessor processor(std::move(is_valid_mac_address_callback));
  for (struct ifaddrs* ifa = ifaddrs; ifa; ifa = ifa->ifa_next) {
    bool keep_going = processor.ProcessInterface(
        ifa, kNetDeviceNamePrefixes, std::size(kNetDeviceNamePrefixes));
    if (!keep_going) {
      break;
    }
  }
  freeifaddrs(ifaddrs);

  return processor.GetMacAddress();
}

void GetRawDeviceIdImpl(IsValidMacAddressCallback is_valid_mac_address_callback,
                        DeviceIdCallback callback) {
  const std::string mac_address =
      GetMacAddress(std::move(is_valid_mac_address_callback));

  const std::string disk_uuid = GetDiskUuid();

  std::string raw_device_id;
  if (!disk_uuid.empty()) {
    raw_device_id = base::StrCat({mac_address, disk_uuid});
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
