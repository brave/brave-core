/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/brave_tor_pluggable_transport_updater.h"

#include <cstdint>
#include <iterator>
#include <string>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/task/task_traits.h"
#include "build/build_config.h"
#include "crypto/sha2.h"

namespace tor {

#if BUILDFLAG(IS_WIN)
constexpr const char kComponentName[] = "Brave Pluggable Transports (Windows)";
constexpr const char kTorPluggableTransportComponentId[] =
    "dnkcahhmfcanmkjhnjejoomdihffoefm";
constexpr uint8_t kComponentPublicKeySHA256[32] = {
    0x3d, 0xa2, 0x07, 0x7c, 0x52, 0x0d, 0xca, 0x97, 0xd9, 0x49, 0xee,
    0xc3, 0x87, 0x55, 0xe4, 0x5c, 0xd2, 0x8f, 0x09, 0xac, 0x4e, 0x37,
    0x4d, 0xdb, 0xa3, 0x1e, 0x30, 0x48, 0x7f, 0xff, 0xa8, 0x70};
static_assert(std::size(kComponentPublicKeySHA256) == crypto::kSHA256Length);
#elif BUILDFLAG(IS_MAC)
constexpr const char kComponentName[] = "Brave Pluggable Transports (Mac)";
constexpr const char kTorPluggableTransportComponentId[] =
    "einfndjnccmoohcngmlldpmellegjjnk";
constexpr uint8_t kComponentPublicKeySHA256[32] = {
    0x48, 0xd5, 0xd3, 0x9d, 0x22, 0xce, 0xe7, 0x2d, 0x6c, 0xbb, 0x3f,
    0xc4, 0xbb, 0x46, 0x99, 0xda, 0x47, 0x09, 0xe4, 0xe9, 0x80, 0x65,
    0xaa, 0xb0, 0xf4, 0xda, 0x33, 0x5c, 0x23, 0x2a, 0x37, 0xd9};
static_assert(std::size(kComponentPublicKeySHA256) == crypto::kSHA256Length);
#elif BUILDFLAG(IS_LINUX)
constexpr const char kComponentName[] = "Brave Pluggable Transports (Linux)";
constexpr const char kTorPluggableTransportComponentId[] =
    "apfggiafobakjahnkchiecbomjgigkkn";
constexpr uint8_t kComponentPublicKeySHA256[32] = {
    0x0f, 0x56, 0x68, 0x05, 0xe1, 0x0a, 0x90, 0x7d, 0xa2, 0x78, 0x42,
    0x1e, 0xc9, 0x68, 0x6a, 0xad, 0x02, 0x8f, 0x08, 0xf0, 0xb1, 0x3e,
    0xcd, 0xac, 0x31, 0xd6, 0x39, 0xd8, 0xe2, 0xeb, 0xc9, 0xc5};
static_assert(std::size(kComponentPublicKeySHA256) == crypto::kSHA256Length);
#endif

constexpr const char kSnowflakeExecutableName[] = "tor-snowflake-brave";
constexpr const char kObfs4ExecutableName[] = "tor-obfs4-brave";

bool Initialize(const base::FilePath& install_dir) {
  const auto executables = {install_dir.AppendASCII(kSnowflakeExecutableName),
                            install_dir.AppendASCII(kObfs4ExecutableName)};

  for (const auto& executable : executables) {
    if (!base::PathExists(executable)) {
      LOG(ERROR) << executable << " doesn't exist";
      return false;
    }
#if BUILDFLAG(IS_POSIX)
    if (!base::SetPosixFilePermissions(executable, 0755)) {
      LOG(ERROR) << "Failed to set executable permission on " << executable;
      return false;
    }
#endif
  }
  return true;
}

BraveTorPluggableTransportUpdater::BraveTorPluggableTransportUpdater(
    BraveComponent::Delegate* component_delegate,
    PrefService* local_state,
    const base::FilePath& user_data_dir)
    : BraveComponent(component_delegate),
      local_state_(local_state),
      user_data_dir_(user_data_dir) {
  DCHECK(local_state);
}

BraveTorPluggableTransportUpdater::~BraveTorPluggableTransportUpdater() =
    default;

void BraveTorPluggableTransportUpdater::Register() {
  if (registered_)
    return;

  BraveComponent::Register(kComponentName, kTorPluggableTransportComponentId,
                           kComponentPublicKeySHA256);
  registered_ = true;
  is_ready_ = false;
}

void BraveTorPluggableTransportUpdater::Unregister() {
  registered_ = false;
  is_ready_ = false;
}

void BraveTorPluggableTransportUpdater::Cleanup() {
  const base::FilePath component_dir =
      user_data_dir_.AppendASCII(kTorPluggableTransportComponentId);
  GetTaskRunner()->PostTask(
      FROM_HERE, base::GetDeletePathRecursivelyCallback(component_dir));
}

bool BraveTorPluggableTransportUpdater::IsReady() const {
  return is_ready_;
}

const base::FilePath&
BraveTorPluggableTransportUpdater::GetSnowflakeExecutable() const {
  return snowflake_path_;
}

const base::FilePath& BraveTorPluggableTransportUpdater::GetObfs4Executable()
    const {
  return obfs4_path_;
}

void BraveTorPluggableTransportUpdater::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveTorPluggableTransportUpdater::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BraveTorPluggableTransportUpdater::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&Initialize, install_dir),
      base::BindOnce(&BraveTorPluggableTransportUpdater::OnInitialized,
                     weak_ptr_factory_.GetWeakPtr(), install_dir));
}

void BraveTorPluggableTransportUpdater::OnInitialized(
    const base::FilePath& install_dir,
    bool success) {
  if (success) {
    // <component_id>/<version>
    const auto relative_component_path =
        base::FilePath::FromASCII(kTorPluggableTransportComponentId)
            .Append(install_dir.BaseName());

    snowflake_path_ =
        relative_component_path.AppendASCII(kSnowflakeExecutableName);
    obfs4_path_ = relative_component_path.AppendASCII(kObfs4ExecutableName);
  } else {
    snowflake_path_.clear();
    obfs4_path_.clear();
  }

  is_ready_ = success;

  for (auto& observer : observers_) {
    observer.OnPluggableTransportReady(success);
  }
}

}  // namespace tor
