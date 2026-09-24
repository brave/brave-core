/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/single_instance.h"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <memory>
#include <utility>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "build/build_config.h"

namespace brave_vpn::v2 {

namespace {
// Linux has Open File Description locks. These are owned by the open file
// description rather than the [process, inode] pair, so an unrelated open/close
// of the lock file elsewhere in the process cannot drop the lock. macOS has no
// real OFD locks (OFD locking is not actually implemented in the Darwin
// kernel), so we fall back to classic fcntl record locks. The lock file must
// stay private to the module, since closing any descriptor to it in this
// process would release the lock.
#if BUILDFLAG(IS_LINUX)
constexpr int kSetLockCommand = F_OFD_SETLK;
#else
constexpr int kSetLockCommand = F_SETLK;
#endif

// Name of the lock file placed inside the caller-supplied user data directory.
constexpr char kLockFileName[] = "agent.lock";

class SingleInstancePosix : public SingleInstance {
 public:
  explicit SingleInstancePosix(base::ScopedFD fd) : fd_(std::move(fd)) {}

 private:
  base::ScopedFD fd_;
};

}  // namespace

// static
std::unique_ptr<SingleInstance> SingleInstance::TryAcquire(
    const base::FilePath& user_data_dir) {
  CHECK(!user_data_dir.empty());
  const base::FilePath path = user_data_dir.Append(kLockFileName);

  // O_CLOEXEC matters in particular for OFD locks: the lock follows the open
  // file description, so a child process that inherited this descriptor would
  // keep the lock alive past our death. O_CLOEXEC prevents the descriptor from
  // surviving an exec, so only this process can hold the lock.
  base::ScopedFD fd(HANDLE_EINTR(
      open(path.value().c_str(),
           O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600)));
  if (!fd.is_valid() && errno == EEXIST) {
    fd.reset(HANDLE_EINTR(
        open(path.value().c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW)));
  }
  if (!fd.is_valid()) {
    // The error could be ELOOP if this is a symlink which might mean a security
    // issue. Symlinks as lock files are not allowed, so we treat this as a
    // failure to acquire the lock.
    PLOG(ERROR) << "Could not open single-instance lock file " << path;
    return nullptr;
  }

  struct flock fl = {};
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;  // 0 == whole file.
  if (HANDLE_EINTR(fcntl(fd.get(), kSetLockCommand, &fl)) == -1) {
    // EAGAIN and EACCES are the only errnos that mean "another process holds a
    // conflicting lock", i.e. another agent is already running. POSIX permits
    // F_SETLK to report contention as either. Everything else is a genuine
    // failure with no second agent involved.
    if (errno == EAGAIN || errno == EACCES) {
      LOG(WARNING) << "Another agent is already running in this session";
    } else {
      PLOG(ERROR) << "Could not acquire single-instance lock on " << path;
    }
    return nullptr;
  }

  return std::make_unique<SingleInstancePosix>(std::move(fd));
}

}  // namespace brave_vpn::v2
