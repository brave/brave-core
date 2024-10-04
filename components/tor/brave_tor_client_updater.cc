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
#include "base/task/task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_switches.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "third_party/re2/src/re2/re2.h"

using brave_component_updater::BraveComponent;

namespace tor {

namespace {

std::pair<base::FilePath, base::FilePath> InitTorPath(
    const base::FilePath& install_dir) {
  base::FilePath executable_path;
  base::FilePath torrc_path;
  base::FileEnumerator traversal(install_dir, false,
                                 base::FileEnumerator::FILES,
                                 FILE_PATH_LITERAL("tor-*"));
  for (base::FilePath current = traversal.Next(); !current.empty();
       current = traversal.Next()) {
    base::FileEnumerator::FileInfo file_info = traversal.GetInfo();
    if (RE2::FullMatch(
            file_info.GetName().MaybeAsASCII(),
            "tor-\\d+\\.\\d+\\.\\d+\\.\\d+-\\w+(-\\w+)?-brave-\\d+")) {
      executable_path = current;
    } else if (file_info.GetName().MaybeAsASCII() == "tor-torrc") {
      torrc_path = current;
    }

    if (!executable_path.empty() && !torrc_path.empty())
      break;
  }

  if (executable_path.empty() || torrc_path.empty()) {
    LOG(ERROR) << "Failed to locate Tor client executable or torrc in "
               << install_dir.value().c_str();
    return std::make_pair(base::FilePath(), base::FilePath());
  }

#if BUILDFLAG(IS_POSIX)
  // Ensure that Tor client executable has appropriate file
  // permissions, as CRX unzipping does not preserve them.
  // See https://crbug.com/555011
  if (!base::SetPosixFilePermissions(executable_path, 0755)) {
    LOG(ERROR) << "Failed to set executable permission on "
               << executable_path.value().c_str();
    return std::make_pair(base::FilePath(), base::FilePath());
  }
#endif  // BUILDFLAG(IS_POSIX)

  return std::make_pair(executable_path, torrc_path);
}

}  // namespace

#if BUILDFLAG(IS_WIN)
constexpr char kTorClientComponentName[] = "Brave Tor Client Updater (Windows)";
constexpr char kTorClientComponentId[] = "cpoalefficncklhjfpglfiplenlpccdb";
constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1AYAsmR/VoRwkZCsjRpD"
    "58xjrgngW5y17H6BqQ7/CeNSpmXlcMXy6bJs2D/yeS96rhZSrQSHTzS9h/ieo/NZ"
    "F5PIwcv07YsG5sRd6zF5a6m92aWCQa1OkbL6jpcpL2Tbc4mCqNxhKMErT7EtIIWL"
    "9cW+mtFUjUjvV3rJLQ3Vy9u6fEi77Y8b25kGnTJoVt3uETAIHBnyNpL7ac2f8Iq+"
    "4Qa6VFmuoBhup54tTZvMv+ikoKKaQkHzkkjTa4hV5AzdnFDKO8C9qJb3T/Ef0+MO"
    "IuZjyySVzGNcOfASeHkhxhlwMQSQuhCN5mdFW5YBnVZ/5QWx8WzbhqBny/ZynS4e"
    "rQIDAQAB";
#elif BUILDFLAG(IS_MAC)
constexpr char kTorClientComponentName[] = "Brave Tor Client Updater (Mac)";
constexpr char kTorClientComponentId[] = "cldoidikboihgcjfkhdeidbpclkineef";
constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAw2QUXSbVuRxYpItYApZ8"
    "Ly/fGeUD3A+vb3J7Ot62CF32wTfWweANWyyB+EBGfbtNDAuRlAbNk0QYeCQEttuf"
    "jLh3Kd5KR5fSyyNNd2cAzAckQ8p7JdiFYjvqZLGC5vlnHgqq4O8xACX5EPwHLNFD"
    "iSpsthNmz3GCUrHrzPHjHVfy+IuucQXygnRv2fwIaAIxJmTbYm4fqsGKpfolWdMe"
    "jKVAy1hc9mApZSyt4oGvUu4SJZnxlYMrY4Ze+OWbDesi2JGy+6dA1ddL9IdnwCb3"
    "9CBOMNjaHeCVz0MKxdCWGPieQM0R7S1KvDCVqAkss6NAbLB6AVM0JulqxC9b+hr/"
    "xwIDAQAB";
#elif BUILDFLAG(IS_LINUX)
constexpr char kTorClientComponentName[] = "Brave Tor Client Updater (Linux)";
#if defined(ARCH_CPU_ARM64)
constexpr char kTorClientComponentId[] = "monolafkoghdlanndjfeebmdfkbklejg";
constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzqb14fggDpbjZtv3HKmR"
    "UTnvfDTcqVbVZo0DdCHQi6SwxDlRweGwsvsHuy9U37VBr41ha/neemQGf+5qkWgY"
    "y+mzzAkb5ZtrHkBSOOsZdyO9WEj7GwXuAx9FvcxG2zPpA/CvagnC14VhMyUFLL8v"
    "XdfHYPmQOtIVdW3eR0G/4JP/mTbnAEkipQfxrDMtDVpX+FDB+Zy5yEMGKWHRLcdH"
    "bHUgb/VhB9ppt0LKRjM44KSpyPDlYquXNcn3WFmxHoVm7PZ3LTAn3eSNZrT4ptmo"
    "KveT4LgWtObrHoZtrg+/LnHAi1GYf8PHrRc+o/FptobOWoUN5lt8NvhLjv85ERBt"
    "rQIDAQAB";
#else
constexpr char kTorClientComponentId[] = "biahpgbdmdkfgndcmfiipgcebobojjkp";
constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAseuq8dXKawkZC7RSE7xb"
    "lRwh6DD+oPEGEjZWKh596/42IrWNQw60gRIR6s7x0YHh5geFnBRkx9bisEXOrFkq"
    "oArVY7eD0gMkjpor9CneD5CnCxc9/2uIPajtXfAmmLAHtN6Wk7yW30SkRf/WvLWX"
    "/H+PqskQBN7I5MO7sveYxSrRMSj7prrFHEiFmXTgG/DwjpzrA7KV6vmzz/ReD51o"
    "+UuLHE7cxPhnsNd/52uY3Lod3GhxvDoXKYx9kWlzBjxB53A2eLBCDIwwCpqS4/Ib"
    "RSJhvF33KQT8YM+7V1MitwB49klP4aEWPXwOlFHmn9Dkmlx2RbO7S0tRcH9UH4LK"
    "2QIDAQAB";
#endif
#endif

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
      weak_ptr_factory_(this) {
  RemoveObsoleteFiles();
}

BraveTorClientUpdater::~BraveTorClientUpdater() = default;

void BraveTorClientUpdater::Register() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (IsTorDisabled() ||
      command_line.HasSwitch(tor::kDisableTorClientUpdaterExtension) ||
      registered_) {
    return;
  }

  BraveComponent::Register(kTorClientComponentName, kTorClientComponentId,
                           kTorClientComponentBase64PublicKey);
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
  task_runner_->PostTask(
      FROM_HERE, base::GetDeletePathRecursivelyCallback(tor_component_dir));
  task_runner_->PostTask(
      FROM_HERE, base::GetDeletePathRecursivelyCallback(GetTorDataPath()));
  task_runner_->PostTask(
      FROM_HERE, base::GetDeletePathRecursivelyCallback(GetTorWatchPath()));
}

void BraveTorClientUpdater::RemoveObsoleteFiles() {
  // tor log
  base::FilePath tor_log = GetTorDataPath().AppendASCII("tor.log");
  task_runner_->PostTask(FROM_HERE, base::GetDeleteFileCallback(tor_log));
}

void BraveTorClientUpdater::SetTorPath(
    const std::pair<base::FilePath, base::FilePath>& paths) {
  executable_path_ = paths.first;
  torrc_path_ = paths.second;
  for (Observer& observer : observers_)
    observer.OnExecutableReady(paths.second);
}

base::FilePath BraveTorClientUpdater::GetExecutablePath() const {
  return executable_path_;
}

base::FilePath BraveTorClientUpdater::GetTorrcPath() const {
  return torrc_path_;
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

void BraveTorClientUpdater::OnComponentReady(const std::string& component_id,
                                             const base::FilePath& install_dir,
                                             const std::string& manifest) {
  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&InitTorPath, install_dir),
      base::BindOnce(&BraveTorClientUpdater::SetTorPath,
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

}  // namespace tor
