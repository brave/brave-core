/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/brave_tor_pluggable_transport_updater.h"

#include <string>

#include "base/bind.h"
#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/task/task_traits.h"
#include "build/build_config.h"

namespace tor {

#if BUILDFLAG(IS_WIN)
constexpr const char kComponentName[] = "Brave Pluggable Transports (Windows)";
#elif BUILDFLAG(IS_MAC)
constexpr const char kComponentName[] = "Brave Pluggable Transports (Mac)";
#elif BUILDFLAG(IS_LINUX)
constexpr const char kComponentName[] = "Brave Pluggable Transports (Linux)";
#endif

constexpr const char kSnowflakeExecutableName[] = "tor-snowflake-brave";
constexpr const char kObfs4ExecutableName[] = "tor-obsf4-brave";

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

constexpr const char kTorPluggableTransportComponentId[] =
    "hajklemofkhfcgodegbighbamaclcfgo";
constexpr const char kComponentBase64PublicKey[] =
    "MIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEA1f30bAR70KT2EJKNxP3O"
    "5lXckE6+Io67Hydjmiou6R5iwkHkrCbelRqLmDpR1i9lR1uLxgc1ZVLKoy0bWeX4"
    "CFu/qwyrWvQ3fnzZ6+AZPk7r2sI56pcj94/RrrVkhvsWwOC6NordeJSLvjlYt+wQ"
    "44UDjobpEaW3p7mz1eXJDNIARL/aI4ARYqWTV2/xtACh1uubSVEo9h7R1TGLTI4A"
    "67X5qtp9FRvgB2O/QHDbdlVXZkhFA3/Bf7k+JonXzpHAbvOXA/QnS8zILYpoOf7d"
    "ySPcI0VBeiH2lM/CX+ZKv+nfOTdBRnWDoE9MxE1eUNWVxdrn/7BsVZh56xnLKO8D"
    "IVkVkE2/e9CNLqSfcAazU672Ro3oA/8XjcyYbLjzbmV3FM85KgEEq6OMlGbSroBi"
    "5KHW/4YhYoRKljhePtuOKmaNNMcZ13iBtiXjZByce+FKfVDw7Lw1wzB7mF4YBfeO"
    "I0AI8JYknZEB8AkrOEMvPvokwXISqRQjkIr7DH3SV4WxAgMBAAE=";

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
                           kComponentBase64PublicKey);
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
      FROM_HERE,
      base::BindOnce(base::GetDeletePathRecursivelyCallback(), component_dir));
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
  return obsf4_path_;
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
    snowflake_path_ = install_dir.AppendASCII(kSnowflakeExecutableName);
    obsf4_path_ = install_dir.AppendASCII(kObfs4ExecutableName);
  } else {
    snowflake_path_.clear();
    obsf4_path_.clear();
  }

  is_ready_ = success;

  for (auto& observer : observers_) {
    observer.OnPluggableTransportReady(success);
  }
}

}  // namespace tor
