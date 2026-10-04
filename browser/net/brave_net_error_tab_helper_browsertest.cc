// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/net/dns_probe_service.h"
#include "chrome/browser/net/dns_probe_service_factory.h"
#include "chrome/browser/net/net_error_tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/embedder_support/pref_names.h"
#include "components/error_page/common/net_error_info.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/dns/public/dns_config_overrides.h"
#include "url/gurl.h"

using chrome_browser_net::DnsProbeService;
using chrome_browser_net::DnsProbeServiceFactory;
using chrome_browser_net::NetErrorTabHelper;

namespace {

constexpr char kDnsFailureHost[] = "dns-failure.test";

// Counts probe requests instead of touching the network.
class CountingDnsProbeService : public DnsProbeService {
 public:
  int probe_count() const { return probe_count_; }

  // DnsProbeService:
  void ProbeDns(ProbeCallback callback) override {
    ++probe_count_;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  error_page::DNS_PROBE_FINISHED_INCONCLUSIVE));
  }
  net::DnsConfigOverrides GetCurrentConfigOverridesForTesting() override {
    return net::DnsConfigOverrides();
  }

 private:
  int probe_count_ = 0;
};

}  // namespace

class BraveNetErrorTabHelperBrowserTest : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    host_resolver()->AddSimulatedFailure(kDnsFailureHost);

    NetErrorTabHelper::set_state_for_testing(
        NetErrorTabHelper::TESTING_DEFAULT);
    // Upstream allows probes when this pref is on.
    profile()->GetPrefs()->SetBoolean(
        embedder_support::kAlternateErrorPagesEnabled, true);

    probe_service_ =
        DnsProbeServiceFactory::GetInstance()
            ->SetTestingSubclassFactoryAndUse<CountingDnsProbeService>(
                profile(), base::BindOnce([](content::BrowserContext*) {
                  return std::make_unique<CountingDnsProbeService>();
                }));
  }

  void TearDownOnMainThread() override {
    probe_service_ = nullptr;
    PlatformBrowserTest::TearDownOnMainThread();
  }

 protected:
  Profile* profile() { return chrome_test_utils::GetProfile(this); }
  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  raw_ptr<CountingDnsProbeService> probe_service_ = nullptr;
};

// Brave does not create NetErrorTabHelper, so no DNS probes are sent
// (brave-browser#14219), even with the pref that upstream requires for probes
// turned on.
IN_PROC_BROWSER_TEST_F(BraveNetErrorTabHelperBrowserTest,
                       DnsErrorDoesNotSendProbe) {
  EXPECT_FALSE(NetErrorTabHelper::From(
      tabs::TabInterface::GetFromContents(web_contents())));

  content::TestNavigationObserver observer(web_contents());
  EXPECT_FALSE(content::NavigateToURL(
      web_contents(), GURL(std::string("https://") + kDnsFailureHost + "/")));
  EXPECT_EQ(net::ERR_NAME_NOT_RESOLVED, observer.last_net_error_code());

  EXPECT_EQ(0, probe_service_->probe_count());
}
