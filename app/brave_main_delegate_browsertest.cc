/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/browser/domain_reliability/service_factory.h"
#include "components/domain_reliability/service.h"

using BraveMainDelegateBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, DomainReliabilityServiceDisabled) {
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableDomainReliability));
  ASSERT_EQ(domain_reliability::DomainReliabilityServiceFactory::GetForBrowserContext(
        ((content::BrowserContext *)browser()->profile())), nullptr);
}
