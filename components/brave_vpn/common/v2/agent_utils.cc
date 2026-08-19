/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/v2/agent_utils.h"

#include "base/check.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

namespace brave_vpn::v2 {
namespace {
constexpr char kAgentServerName[] = "brave_vpn_agent";

#if BUILDFLAG(IS_POSIX)
// Returns the directory holding the agent's socket. This is where the
// per-session scoping comes from on POSIX: both candidate directories are
// already private to one login session, mode 0700, so the socket's own name can
// be a constant.
base::FilePath GetSocketDirectory() {
#if BUILDFLAG(IS_MAC)
  // The per-user confined NSTemporaryDirectory(), distinct for every user and
  // unreadable by others, which is what "per session" means in practice on
  // macOS, where concurrent GUI logins are per-user.
  return base::PathService::CheckedGet(base::DIR_TEMP);
#elif BUILDFLAG(IS_LINUX)
  // /run/user/<uid>: mode 0700, created by pam_systemd at login and removed at
  // last logout. Reading XDG_RUNTIME_DIR specifically: systemd exports
  // XDG_RUNTIME_DIR to both session scopes and to `user@<uid>.service` units,
  // so a systemd user unit and the browser agree on it.
  auto env = base::Environment::Create();
  std::string runtime_dir =
      env->GetVar("XDG_RUNTIME_DIR").value_or(std::string());
  if (runtime_dir.empty()) {
    LOG(ERROR)
        << "XDG_RUNTIME_DIR is not set; the agent requires a GUI session";
    return base::FilePath();
  }
  return base::FilePath(runtime_dir);
#else
#error unsupported platform
#endif
}
#endif  // BUILDFLAG(IS_POSIX)

}  // namespace

std::optional<mojo::NamedPlatformChannel::ServerName> GetAgentServerName() {
#if BUILDFLAG(IS_WIN)
  // Keep the name free of anything user-identifying: any process on the machine
  // can enumerate the pipe namespace.
  DWORD session_id = 0;
  // Cannot fail when querying the calling process's own id.
  CHECK(::ProcessIdToSessionId(::GetCurrentProcessId(), &session_id));
  return base::ASCIIToWide(
      base::StrCat({kAgentServerName, ".", base::NumberToString(session_id)}));
#else
  return GetAgentServerNameForDirectory(GetSocketDirectory());
#endif  // BUILDFLAG(IS_WIN)
}

#if BUILDFLAG(IS_POSIX)

std::optional<mojo::NamedPlatformChannel::ServerName>
GetAgentServerNameForDirectory(const base::FilePath& socket_dir) {
  if (socket_dir.empty()) {
    return std::nullopt;
  }

  // On POSIX mojo::NamedPlatformChannel uses the ServerName verbatim as the
  // sockaddr_un path, so it must be absolute. A bare name would bind() into
  // whatever the process's current working directory happens to be.
  const base::FilePath path = socket_dir.Append(kAgentServerName);

  // Deliberately no truncation or fallback: an over-long path makes bind() and
  // connect() fail identically on both sides, which is a loud and symmetric
  // failure.
  if (path.value().size() > kMaxAgentSocketPathLength) {
    LOG(ERROR) << "Agent socket path exceeds the sockaddr_un limit: " << path;
    return std::nullopt;
  }
  return path.value();
}

#endif  // BUILDFLAG(IS_POSIX)

}  // namespace brave_vpn::v2
