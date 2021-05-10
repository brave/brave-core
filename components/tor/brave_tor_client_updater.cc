/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/brave_tor_client_updater.h"

#include <memory>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/task_runner.h"
#include "base/task_runner_util.h"
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_switches.h"
#include "components/prefs/pref_service.h"
#include "third_party/re2/src/re2/re2.h"

using brave_component_updater::BraveComponent;

namespace tor {

namespace {

base::FilePath InitExecutablePath(const base::FilePath& install_dir) {
  base::FilePath executable_path;
  base::FileEnumerator traversal(install_dir, false,
                                 base::FileEnumerator::FILES,
                                 FILE_PATH_LITERAL("tor-*"));
  for (base::FilePath current = traversal.Next(); !current.empty();
       current = traversal.Next()) {
    base::FileEnumerator::FileInfo file_info = traversal.GetInfo();
    if (!RE2::FullMatch(file_info.GetName().MaybeAsASCII(),
                        "tor-\\d+\\.\\d+\\.\\d+\\.\\d+-\\w+-brave-\\d+"))
      continue;
    executable_path = current;
    break;
  }

  if (executable_path.empty()) {
    LOG(ERROR) << "Failed to locate Tor client executable in "
               << install_dir.value().c_str();
    return base::FilePath();
  }

#if defined(OS_POSIX)
  // Ensure that Tor client executable has appropriate file
  // permissions, as CRX unzipping does not preserve them.
  // See https://crbug.com/555011
  if (!base::SetPosixFilePermissions(executable_path, 0755)) {
    LOG(ERROR) << "Failed to set executable permission on "
               << executable_path.value().c_str();
    return base::FilePath();
  }
#endif  // defined(OS_POSIX)

  return executable_path;
}

void DeleteDir(const base::FilePath& path) {
  base::DeletePathRecursively(path);
}

}  // namespace

#if defined(OS_WIN)
const char kTorClientComponentName[] = "Brave Tor Client Updater (Windows)";
const char kTorClientComponentId[] = "cpoalefficncklhjfpglfiplenlpccdb";
const char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1AYAsmR/VoRwkZCsjRpD"
    "58xjrgngW5y17H6BqQ7/CeNSpmXlcMXy6bJs2D/yeS96rhZSrQSHTzS9h/ieo/NZ"
    "F5PIwcv07YsG5sRd6zF5a6m92aWCQa1OkbL6jpcpL2Tbc4mCqNxhKMErT7EtIIWL"
    "9cW+mtFUjUjvV3rJLQ3Vy9u6fEi77Y8b25kGnTJoVt3uETAIHBnyNpL7ac2f8Iq+"
    "4Qa6VFmuoBhup54tTZvMv+ikoKKaQkHzkkjTa4hV5AzdnFDKO8C9qJb3T/Ef0+MO"
    "IuZjyySVzGNcOfASeHkhxhlwMQSQuhCN5mdFW5YBnVZ/5QWx8WzbhqBny/ZynS4e"
    "rQIDAQAB";
#elif defined(OS_MAC)
const char kTorClientComponentName[] = "Brave Tor Client Updater (Mac)";
const char kTorClientComponentId[] = "cldoidikboihgcjfkhdeidbpclkineef";
const char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAw2QUXSbVuRxYpItYApZ8"
    "Ly/fGeUD3A+vb3J7Ot62CF32wTfWweANWyyB+EBGfbtNDAuRlAbNk0QYeCQEttuf"
    "jLh3Kd5KR5fSyyNNd2cAzAckQ8p7JdiFYjvqZLGC5vlnHgqq4O8xACX5EPwHLNFD"
    "iSpsthNmz3GCUrHrzPHjHVfy+IuucQXygnRv2fwIaAIxJmTbYm4fqsGKpfolWdMe"
    "jKVAy1hc9mApZSyt4oGvUu4SJZnxlYMrY4Ze+OWbDesi2JGy+6dA1ddL9IdnwCb3"
    "9CBOMNjaHeCVz0MKxdCWGPieQM0R7S1KvDCVqAkss6NAbLB6AVM0JulqxC9b+hr/"
    "xwIDAQAB";
#elif defined(OS_LINUX)
const char kTorClientComponentName[] = "Brave Tor Client Updater (Linux)";
const char kTorClientComponentId[] = "biahpgbdmdkfgndcmfiipgcebobojjkp";
const char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAseuq8dXKawkZC7RSE7xb"
    "lRwh6DD+oPEGEjZWKh596/42IrWNQw60gRIR6s7x0YHh5geFnBRkx9bisEXOrFkq"
    "oArVY7eD0gMkjpor9CneD5CnCxc9/2uIPajtXfAmmLAHtN6Wk7yW30SkRf/WvLWX"
    "/H+PqskQBN7I5MO7sveYxSrRMSj7prrFHEiFmXTgG/DwjpzrA7KV6vmzz/ReD51o"
    "+UuLHE7cxPhnsNd/52uY3Lod3GhxvDoXKYx9kWlzBjxB53A2eLBCDIwwCpqS4/Ib"
    "RSJhvF33KQT8YM+7V1MitwB49klP4aEWPXwOlFHmn9Dkmlx2RbO7S0tRcH9UH4LK"
    "2QIDAQAB";
#endif

std::string BraveTorClientUpdater::g_tor_client_component_id_(
    kTorClientComponentId);
std::string BraveTorClientUpdater::g_tor_client_component_base64_public_key_(
    kTorClientComponentBase64PublicKey);

BraveTorClientUpdater::BraveTorClientUpdater(
    BraveComponent::Delegate* component_delegate,
    PrefService* local_state,
    const base::FilePath& user_data_dir)
    : BraveComponent(component_delegate),
      task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()})),
      registered_(false),
      local_state_(local_state),
      user_data_dir_(user_data_dir),
      weak_ptr_factory_(this) {}

BraveTorClientUpdater::~BraveTorClientUpdater() {}

void BraveTorClientUpdater::Register() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (IsTorDisabled() ||
      command_line.HasSwitch(tor::kDisableTorClientUpdaterExtension) ||
      registered_) {
    return;
  }

  BraveComponent::Register(kTorClientComponentName,
                           g_tor_client_component_id_,
                           g_tor_client_component_base64_public_key_);
  registered_ = true;
}

void BraveTorClientUpdater::Unregister() {
  // We don't call BraveComponent::Unregister here in order to prevent tor
  // executable component from getting deleted when last tor window closed
  registered_ = false;
}

void BraveTorClientUpdater::Cleanup() {
  DCHECK(!user_data_dir_.empty());
  base::FilePath tor_component_dir =
      user_data_dir_.AppendASCII(kTorClientComponentId);
  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&DeleteDir, tor_component_dir));
  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&DeleteDir, GetTorDataPath()));
  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&DeleteDir, GetTorWatchPath()));
}

void BraveTorClientUpdater::SetExecutablePath(const base::FilePath& path) {
  executable_path_ = path;
  for (Observer& observer : observers_)
    observer.OnExecutableReady(path);
}

base::FilePath BraveTorClientUpdater::GetExecutablePath() const {
  return executable_path_;
}

base::FilePath BraveTorClientUpdater::GetTorDataPath() const {
  DCHECK(!user_data_dir_.empty());
  return user_data_dir_.Append(FILE_PATH_LITERAL("tor"))
      .Append(FILE_PATH_LITERAL("data"));
}

base::FilePath BraveTorClientUpdater::GetTorWatchPath() const {
  DCHECK(!user_data_dir_.empty());
  return user_data_dir_.Append(FILE_PATH_LITERAL("tor"))
      .Append(FILE_PATH_LITERAL("watch"));
}

void BraveTorClientUpdater::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  base::PostTaskAndReplyWithResult(
      GetTaskRunner().get(), FROM_HERE,
      base::BindOnce(&InitExecutablePath, install_dir),
      base::BindOnce(&BraveTorClientUpdater::SetExecutablePath,
                     weak_ptr_factory_.GetWeakPtr()));
}

bool BraveTorClientUpdater::IsTorDisabled() {
  if (local_state_)
    return local_state_->GetBoolean(tor::prefs::kTorDisabled);
  return false;
}

void BraveTorClientUpdater::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveTorClientUpdater::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// static
void BraveTorClientUpdater::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_tor_client_component_id_ = component_id;
  g_tor_client_component_base64_public_key_ = component_base64_public_key;
}

}  // namespace tor
