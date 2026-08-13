/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/v2/agent_utils.h"

#include <stddef.h>

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/scoped_environment_variable_override.h"
#include "build/build_config.h"
#include "mojo/public/cpp/platform/named_platform_channel.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {
#if BUILDFLAG(IS_POSIX)
constexpr char kSocketRuntimeDir[] = "/run/user/1000";
#endif
}  // namespace

// The browser and the agent call this independently and must agree, so the
// answer cannot depend on anything that varies between calls.
TEST(BraveVpnAgentUtils, ServerNameIsStableWithinProcess) {
#if BUILDFLAG(IS_LINUX)
  // Test bots frequently have no XDG_RUNTIME_DIR, which would leave both calls
  // below as nullopt and pass without a name ever being built. Safe to override
  // here because these tests start no threads.
  base::ScopedEnvironmentVariableOverride runtime_dir("XDG_RUNTIME_DIR",
                                                      kSocketRuntimeDir);
#endif  // BUILDFLAG(IS_LINUX)

  const std::optional<mojo::NamedPlatformChannel::ServerName> first =
      GetAgentServerName();
  const std::optional<mojo::NamedPlatformChannel::ServerName> second =
      GetAgentServerName();

  ASSERT_TRUE(first.has_value());
  EXPECT_EQ(first, second);
}

#if BUILDFLAG(IS_LINUX)

// Without a runtime directory there is no path for the browser and the agent to
// agree on, so the name has to be refused rather than resolved somewhere else.
TEST(BraveVpnAgentUtils, NoRuntimeDirectoryYieldsNoServerName) {
  base::ScopedEnvironmentVariableOverride unset_runtime_dir("XDG_RUNTIME_DIR");
  EXPECT_FALSE(GetAgentServerName().has_value());
}

#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_POSIX)

// An unusable socket directory has to fail rather than fall back to a
// relative path, which would bind into the working directory.
TEST(BraveVpnAgentUtils, NoSocketDirectoryYieldsNoServerName) {
  EXPECT_FALSE(GetAgentServerNameForDirectory(base::FilePath()).has_value());
}

TEST(BraveVpnAgentUtils, ServerNameIsAnAbsolutePathInsideTheDirectory) {
  const base::FilePath dir(kSocketRuntimeDir);

  const std::optional<mojo::NamedPlatformChannel::ServerName> name =
      GetAgentServerNameForDirectory(dir);
  ASSERT_TRUE(name.has_value());

  const base::FilePath path(*name);
  EXPECT_TRUE(path.IsAbsolute());
  EXPECT_EQ(dir, path.DirName());
  EXPECT_FALSE(path.BaseName().empty());
}

// The same leaf name must come back for every directory, or the browser and the
// agent could disagree about which socket to use.
TEST(BraveVpnAgentUtils, LeafNameDoesNotDependOnTheDirectory) {
  const std::optional<mojo::NamedPlatformChannel::ServerName> first =
      GetAgentServerNameForDirectory(base::FilePath(kSocketRuntimeDir));
  const std::optional<mojo::NamedPlatformChannel::ServerName> second =
      GetAgentServerNameForDirectory(base::FilePath("/tmp"));

  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  EXPECT_EQ(base::FilePath(*first).BaseName(),
            base::FilePath(*second).BaseName());
}

// bind() and connect() fail identically on an over-long path, so the limit is
// enforced up front. Pinning both sides of it guards against an off-by-one.
TEST(BraveVpnAgentUtils, SocketPathLengthLimitIsEnforcedExactly) {
  // Calculate the leaf's contribution.
  const base::FilePath probe("/probe");
  const std::optional<mojo::NamedPlatformChannel::ServerName> probe_name =
      GetAgentServerNameForDirectory(probe);
  ASSERT_TRUE(probe_name.has_value());
  const size_t leaf_size = probe_name->size() - probe.value().size();
  ASSERT_LT(leaf_size, kMaxAgentSocketPathLength);

  // A directory that lands the socket exactly on the limit is usable.
  const size_t longest_dir = kMaxAgentSocketPathLength - leaf_size;
  const base::FilePath longest("/" + std::string(longest_dir - 1, 'a'));
  ASSERT_EQ(longest_dir, longest.value().size());
  const std::optional<mojo::NamedPlatformChannel::ServerName> accepted =
      GetAgentServerNameForDirectory(longest);
  ASSERT_TRUE(accepted.has_value());
  EXPECT_EQ(kMaxAgentSocketPathLength, accepted->size());

  // One byte more is refused.
  const std::optional<mojo::NamedPlatformChannel::ServerName> rejected =
      GetAgentServerNameForDirectory(base::FilePath(longest.value() + "a"));
  EXPECT_FALSE(rejected.has_value());
}

#else  // BUILDFLAG(IS_POSIX)

TEST(BraveVpnAgentUtils, ServerNameIsAvailableOnNonPosix) {
  const std::optional<mojo::NamedPlatformChannel::ServerName> name =
      GetAgentServerName();
  ASSERT_TRUE(name.has_value());
  EXPECT_FALSE(name->empty());
}

#endif  // BUILDFLAG(IS_POSIX)

}  // namespace brave_vpn::v2
