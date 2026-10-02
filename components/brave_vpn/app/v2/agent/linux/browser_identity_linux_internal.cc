/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/linux/browser_identity_linux_internal.h"

#include <sys/syscall.h>
#include <unistd.h>

#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"

// Could be missing from older sysroot headers. Both numbers are the same on
// every architecture.
#ifndef SYS_pidfd_send_signal
#define SYS_pidfd_send_signal 424
#endif
#ifndef SYS_pidfd_open
#define SYS_pidfd_open 434
#endif

namespace brave_vpn::v2::internal {
namespace {
// Fields of "/proc/<pid>/stat", numbered from 1 as in proc_pid_stat(5).
constexpr size_t kStateField = 3;
constexpr size_t kStartTimeField = 22;
}  // namespace

base::ScopedFD OpenPidfd(pid_t pid) {
  return base::ScopedFD(static_cast<int>(syscall(SYS_pidfd_open, pid, 0)));
}

bool IsProcessAlive(const base::ScopedFD& pidfd) {
  return syscall(SYS_pidfd_send_signal, pidfd.get(), 0, nullptr, 0) == 0;
}

std::optional<uint64_t> ReadProcessStartTimeTicks(pid_t pid) {
  const base::FilePath stat_path =
      base::FilePath("/proc").Append(base::NumberToString(pid)).Append("stat");
  // Unlike ReadFileToString(), this does not assert that blocking is allowed:
  // reads from /proc never block.
  std::string stat_contents;
  if (!base::ReadFileToStringNonBlocking(stat_path, &stat_contents)) {
    return std::nullopt;
  }
  return ParseProcessStartTimeTicks(stat_contents);
}

std::optional<uint64_t> ParseProcessStartTimeTicks(
    std::string_view stat_contents) {
  // Field 2 (comm) is parenthesized and may itself contain spaces and
  // parentheses, so the fields after it begin after the last ')'.
  const auto comm_split = base::RSplitStringOnce(stat_contents, ')');
  if (!comm_split) {
    return std::nullopt;
  }
  const std::vector<std::string_view> fields =
      base::SplitStringPiece(comm_split->second, " ", base::TRIM_WHITESPACE,
                             base::SPLIT_WANT_NONEMPTY);

  // fields[0] is the state field.
  constexpr size_t kStartTimeIndex = kStartTimeField - kStateField;
  uint64_t start_time_ticks = 0;
  if (kStartTimeIndex >= fields.size() ||
      !base::StringToUint64(fields[kStartTimeIndex], &start_time_ticks)) {
    return std::nullopt;
  }
  return start_time_ticks;
}

}  // namespace brave_vpn::v2::internal
