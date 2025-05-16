/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/lookalikes/lookalike_url_navigation_throttle.h"

#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/lookalikes/core/safety_tip_test_utils.h"
#include "components/url_formatter/spoof_checks/idn_spoof_checker.h"
#include "components/url_formatter/url_formatter.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/mock_navigation_throttle_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace lookalikes {

class LookalikeThrottleTest : public ChromeRenderViewHostTestHarness {};

// Tests that spoofy hostnames are properly handled in the throttle.
TEST_F(LookalikeThrottleTest, Basics) {
  lookalikes::InitializeSafetyTipConfig();

  const struct TestCase {
    const char* hostname;
    bool expected_blocked;
    url_formatter::IDNSpoofChecker::Result expected_spoof_check_result;
  } kTestCases[] = {
      // ASCII private domain.
      {"private.hostname", false,
       url_formatter::IDNSpoofChecker::Result::kNone},

      // lɔlocked.com, fails ICU spoof checks.
      {"xn--llocked-9bd.com", true,
       url_formatter::IDNSpoofChecker::Result::kICUSpoofChecks},
      // þook.com, contains a TLD specific character (þ).
      {"xn--ook-ooa.com", true,
       url_formatter::IDNSpoofChecker::Result::kTLDSpecificCharacters},
      // example·com.com, unsafe middle dot.
      {"xn--examplecom-rra.com", true,
       url_formatter::IDNSpoofChecker::Result::kUnsafeMiddleDot},
      // scope.com, with scope in Cyrillic. Whole script confusable.
      {"xn--e1argc3h.com", true,
       url_formatter::IDNSpoofChecker::Result::kWholeScriptConfusable},
      //  Non-ASCII Latin with Non-Latin character
      {"xn--caf-dma9024xvpg.kr", true,
       url_formatter::IDNSpoofChecker::Result::
           kNonAsciiLatinCharMixedWithNonLatin},
      // testーsite.com, has dangerous pattern (ー is CJK character).
      {"xn--testsite-1g5g.com", true,
       url_formatter::IDNSpoofChecker::Result::kDangerousPattern},

      // 🍕.com, fails ICU spoof checks, but is allowed because consists of only
      // emoji and ASCII.
      {"xn--vi8h.com", false,
       url_formatter::IDNSpoofChecker::Result::kICUSpoofChecks},
      // sparkasse-gießen.de, has a deviation character (ß). This is in punycode
      // because GURL canonicalizes ß to ss.
      {"xn--sparkasse-gieen-2ib.de", false,
       url_formatter::IDNSpoofChecker::Result::kSafe},
  };

  for (const TestCase& test_case : kTestCases) {
    url_formatter::IDNConversionResult idn_result =
        url_formatter::UnsafeIDNToUnicodeWithDetails(test_case.hostname);
    ASSERT_EQ(test_case.expected_spoof_check_result,
              idn_result.spoof_check_result)
        << test_case.hostname;

    GURL url(std::string("http://") + test_case.hostname);
    ::testing::NiceMock<content::MockNavigationHandle> handle(url, main_rfh());
    handle.set_redirect_chain({url});
    handle.set_page_transition(ui::PAGE_TRANSITION_TYPED);
    ::testing::NiceMock<content::MockNavigationThrottleRegistry> registry(
        &handle,
        content::MockNavigationThrottleRegistry::RegistrationMode::kHold);

    LookalikeUrlNavigationThrottle::MaybeCreateAndAdd(registry);
    ASSERT_EQ(0u, registry.throttles().size());
  }
}

}  // namespace lookalikes
