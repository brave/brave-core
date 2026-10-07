/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/v2/agent_utils.h"

#include <string>
#include <string_view>

#include "base/check.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/common/v2/branding_buildflags.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#elif BUILDFLAG(IS_MAC)
#include <servers/bootstrap.h>
#endif

namespace brave_vpn::v2 {
namespace {
constexpr char kAgentServerName[] = BUILDFLAG(VPN_AGENT_IPC_NAME);

#if BUILDFLAG(IS_MAC)
// Mojo only DCHECKs this, so a release build would silently register a
// truncated or rejected name.
static_assert(std::string_view(kAgentServerName).size() <
                  static_cast<size_t>(BOOTSTRAP_MAX_NAME_LEN),
              "Agent bootstrap name exceeds BOOTSTRAP_MAX_NAME_LEN");
#endif  // BUILDFLAG(IS_MAC)

#if BUILDFLAG(IS_LINUX)
// Returns the directory holding the agent's socket. This is where the
// per-session scoping comes from: a candidate directory is already private to
// one login session, mode 0700, so the socket's own name can be a constant.
// Reading XDG_RUNTIME_DIR specifically: systemd exports it to both session
// scopes and to `user@<uid>.service` units, so a systemd user unit and the
// browser agree on it.
base::FilePath GetSocketDirectory() {
  auto env = base::Environment::Create();
  std::string runtime_dir =
      env->GetVar("XDG_RUNTIME_DIR").value_or(std::string());
  if (runtime_dir.empty()) {
    LOG(ERROR)
        << "XDG_RUNTIME_DIR is not set; the agent requires a GUI session";
    return base::FilePath();
  }
  return base::FilePath(runtime_dir);
}
#endif  // BUILDFLAG(IS_LINUX)

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
#elif BUILDFLAG(IS_MAC)
  // Deliberately not scoped by a directory: the bootstrap namespace is already
  // per-user and per-session, so a path prefix would add no benefit.
  return std::string(kAgentServerName);
#elif BUILDFLAG(IS_LINUX)
  return GetAgentServerNameForDirectory(GetSocketDirectory());
#else
#error unsupported platform
#endif
}

#if BUILDFLAG(IS_LINUX)

std::optional<mojo::NamedPlatformChannel::ServerName>
GetAgentServerNameForDirectory(const base::FilePath& socket_dir) {
  // Mojo uses the ServerName verbatim as the sockaddr_un path, so it must be
  // absolute. A bare name would bind() into whatever the process's current
  // working directory happens to be.
  if (socket_dir.empty() || !socket_dir.IsAbsolute()) {
    return std::nullopt;
  }

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

#endif  // BUILDFLAG(IS_LINUX)

}  // namespace brave_vpn::v2
