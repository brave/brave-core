/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/linux/browser_identity_linux_internal.h"

#include <sys/syscall.h>
#include <unistd.h>

#include <cerrno>
#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

// Could be missing from older sysroot headers. These are the numbers in the
// unified syscall table that every architecture Brave targets uses.
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

// "Uid:" line of "/proc/<pid>/status": real, effective, saved set and
// filesystem uids, in that order.
constexpr std::string_view kUidPrefix = "Uid:";
constexpr size_t kUidCount = 4;
constexpr size_t kEffectiveUidIndex = 1;

std::optional<std::string> ReadProcFile(pid_t pid, std::string_view name) {
  const base::FilePath path =
      base::FilePath("/proc").Append(base::NumberToString(pid)).Append(name);
  // Unlike ReadFileToString(), this does not assert that blocking is allowed:
  // reads from /proc never block.
  std::string contents;
  if (!base::ReadFileToStringNonBlocking(path, &contents)) {
    return std::nullopt;
  }
  return contents;
}
}  // namespace

base::ScopedFD OpenPidfd(pid_t pid) {
  // TODO(https://github.com/brave/brave-browser/issues/54635)
  // pidfd_open needs kernel 5.3. On older kernels (e.g. Debian 10
  // on 4.19, Ubuntu 18.04 on 4.15), every connection will be refused. Whether
  // we accept this behaviour or not is a product decision to be made by the
  // time we revisit Linux support.
  return base::ScopedFD(static_cast<int>(syscall(SYS_pidfd_open, pid, 0)));
}

bool IsProcessAlive(const base::ScopedFD& pidfd) {
  if (syscall(SYS_pidfd_send_signal, pidfd.get(), 0, nullptr, 0) == 0) {
    return true;
  }
  // The kernel checks permission only after finding the target, so EPERM
  // means the process is unreaped but may not be signalled by us.
  return errno == EPERM;
}

std::optional<uint64_t> ReadProcessStartTimeTicks(pid_t pid) {
  const std::optional<std::string> stat_contents = ReadProcFile(pid, "stat");
  if (!stat_contents) {
    return std::nullopt;
  }
  return ParseProcessStartTimeTicks(*stat_contents);
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

std::optional<uid_t> ReadProcessEffectiveUid(pid_t pid) {
  const std::optional<std::string> status_contents =
      ReadProcFile(pid, "status");
  if (!status_contents) {
    return std::nullopt;
  }
  return ParseProcessEffectiveUid(*status_contents);
}

std::optional<uid_t> ParseProcessEffectiveUid(
    std::string_view status_contents) {
  // The process controls its own "Name:" line, but the kernel escapes newlines
  // in it, so it cannot add a line of its own. More than one "Uid:" line is
  // refused anyway rather than trusting either.
  std::optional<std::string_view> uid_fields;
  for (std::string_view line :
       base::SplitStringPiece(status_contents, "\n", base::KEEP_WHITESPACE,
                              base::SPLIT_WANT_NONEMPTY)) {
    if (!line.starts_with(kUidPrefix)) {
      continue;
    }
    if (uid_fields) {
      return std::nullopt;
    }
    uid_fields = line.substr(kUidPrefix.size());
  }
  if (!uid_fields) {
    return std::nullopt;
  }

  const std::vector<std::string_view> uids =
      base::SplitStringPiece(*uid_fields, base::kWhitespaceASCII,
                             base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  unsigned effective_uid = 0;
  if (uids.size() != kUidCount ||
      !base::StringToUint(uids[kEffectiveUidIndex], &effective_uid)) {
    return std::nullopt;
  }
  return static_cast<uid_t>(effective_uid);
}

}  // namespace brave_vpn::v2::internal
