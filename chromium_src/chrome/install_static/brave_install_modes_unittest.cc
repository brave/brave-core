/* Copyright (c) 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "chrome/install_static/install_modes.h"

#include <windows.h>  // NOLINT

#include <cguid.h>  // NOLINT

#include "base/strings/string_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::Le;
using ::testing::Ne;
using ::testing::Not;
using ::testing::ResultOf;
using ::testing::StrEq;
using ::testing::StrNe;

namespace install_static {

namespace {

// A matcher that returns true if |arg| contains a character that is neither
// alpha-numeric nor a period.
MATCHER(ContainsIllegalProgIdChar, "") {
  const wchar_t* scan = arg;
  wchar_t c;
  while ((c = *scan++) != 0) {
    if (!base::IsAsciiAlphaNumeric(c) && c != L'.') {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST(InstallModes, VerifyModes) {
  ASSERT_THAT(NUM_INSTALL_MODES, Gt(0));
  for (int i = 0; i < NUM_INSTALL_MODES; ++i) {
    const InstallConstants& mode = kInstallModes[i];

    // The modes must be listed in order.
    ASSERT_THAT(mode.index, Eq(i));

    // The first mode must have no install switch; the rest must have one.
    if (i == 0)
      ASSERT_THAT(mode.install_switch, StrEq(""));
    else
      ASSERT_THAT(mode.install_switch, StrNe(""));

    // The first mode must have no suffix; the rest must have one.
    if (i == 0)
      ASSERT_THAT(mode.install_suffix, StrEq(L""));
    else
      ASSERT_THAT(mode.install_suffix, StrNe(L""));

    // The first mode must have no logo suffix; the rest must have one.
    if (i == 0)
      ASSERT_THAT(mode.logo_suffix, StrEq(L""));
    else
      ASSERT_THAT(mode.logo_suffix, StrNe(L""));

    // The modes must have an appguid if Google Update integration is supported.
#if defined(OFFICIAL_BUILD)
      ASSERT_THAT(mode.app_guid, StrNe(L""));
#else
      ASSERT_THAT(mode.app_guid, StrEq(L""));
#endif

    // Every mode must have a base app name.
    ASSERT_THAT(mode.base_app_name, StrNe(L""));

    // Every mode must have a base app id.
    ASSERT_THAT(mode.base_app_id, StrNe(L""));

    // The ProgID prefix must not be empty, must be no greater than 11
    // characters long, must contain no punctuation, and may not start with a
    // digit (https://msdn.microsoft.com/library/windows/desktop/dd542719.aspx).
    ASSERT_THAT(mode.browser_prog_id_prefix, StrNe(L""));
    ASSERT_THAT(lstrlen(mode.browser_prog_id_prefix), Le(11));
    ASSERT_THAT(mode.browser_prog_id_prefix, Not(ContainsIllegalProgIdChar()));
    ASSERT_THAT(*mode.browser_prog_id_prefix, ResultOf(iswdigit, Eq(0)));

    // The ProgID description must not be empty.
    ASSERT_THAT(mode.browser_prog_id_description, StrNe(L""));

    // Every mode must have an Active Setup GUID.
    ASSERT_THAT(mode.active_setup_guid, StrNe(L""));

    // Every mode must have a toast activator CLSID.
    ASSERT_THAT(mode.toast_activator_clsid, Ne(CLSID_NULL));

    // UNSUPPORTED and BUILDFLAG(USE_GOOGLE_UPDATE_INTEGRATION) which we set for
    // OFFICIAL_BUILD are mutually exclusive.
#if defined(OFFICIAL_BUILD)
    ASSERT_THAT(mode.channel_strategy,
                AnyOf(ChannelStrategy::FLOATING, ChannelStrategy::FIXED));
#else
    ASSERT_THAT(mode.channel_strategy, Eq(ChannelStrategy::UNSUPPORTED));
#endif
  }
}

TEST(InstallModes, GetClientsKeyPath) {
  constexpr wchar_t kAppGuid[] = L"test";

#if defined(OFFICIAL_BUILD)
    ASSERT_THAT(GetClientsKeyPath(kAppGuid),
                StrEq(L"Software\\BraveSoftware\\Update\\Clients\\test"));
#else
    ASSERT_THAT(GetClientsKeyPath(kAppGuid),
                StrEq(std::wstring(L"Software\\").append(kProductPathName)));
#endif
}

TEST(InstallModes, GetClientStateKeyPath) {
  constexpr wchar_t kAppGuid[] = L"test";

#if defined(OFFICIAL_BUILD)
    ASSERT_THAT(GetClientStateKeyPath(kAppGuid),
                StrEq(L"Software\\BraveSoftware\\Update\\ClientState\\test"));
#else
    ASSERT_THAT(GetClientStateKeyPath(kAppGuid),
                StrEq(std::wstring(L"Software\\").append(kProductPathName)));
#endif
}

TEST(InstallModes, GetClientStateMediumKeyPath) {
  constexpr wchar_t kAppGuid[] = L"test";

#if defined(OFFICIAL_BUILD)
    ASSERT_THAT(
        GetClientStateMediumKeyPath(kAppGuid),
        StrEq(L"Software\\BraveSoftware\\Update\\ClientStateMedium\\test"));
#else
    ASSERT_THAT(GetClientStateMediumKeyPath(kAppGuid),
                StrEq(std::wstring(L"Software\\").append(kProductPathName)));
#endif
}

#if defined(OFFICIAL_BUILD)
TEST(InstallModes, NightlyModesTest) {
  EXPECT_TRUE(kInstallModes[NIGHTLY_INDEX].supports_system_level);
  EXPECT_TRUE(kInstallModes[NIGHTLY_INDEX].supports_set_as_default_browser);
}
#endif

}  // namespace install_static
