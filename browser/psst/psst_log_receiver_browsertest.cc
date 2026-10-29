// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_log_receiver.h"

#include <string>
#include <vector>

#include "base/location.h"
#include "base/strings/strcat.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "brave/browser/psst/psst_log_service_factory.h"
#include "brave/components/psst/core/browser/psst_log_service.h"
#include "brave/components/psst/core/common/constants.h"
#include "brave/components/psst/core/common/features.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace psst {

namespace {

constexpr char kMainFrameHost[] = "a.test";
constexpr char kSubFrameHost[] = "b.test";

}  // namespace

// Covers the renderer -> browser half of PSST logging: that `psstLog` exists
// only in the PSST isolated world, and that what it sends is attributed by the
// browser rather than by the renderer.
class PsstLogReceiverBrowserTest : public PlatformBrowserTest {
 public:
  PsstLogReceiverBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(features::kEnablePsst);
  }
  ~PsstLogReceiverBrowserTest() override = default;

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());
  }

 protected:
  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  PsstLogService* log_service() {
    return PsstLogServiceFactory::GetForProfile(
        chrome_test_utils::GetProfile(this));
  }

  // Navigates to a page on `host` that embeds a cross-site iframe.
  void NavigateToPageWithIframe(
      const std::string& host,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    ASSERT_TRUE(content::NavigateToURL(
        web_contents(), https_server_.GetURL(host, "/iframe_cross_site.html")));
  }

  // Runs `script` in the PSST isolated world, which also creates that world's
  // script context and so triggers installation of `psstLog`.
  content::EvalJsResult EvalInPsstWorld(
      const content::ToRenderFrameHost& target,
      const std::string& script) {
    return content::EvalJs(target, script,
                           content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                           ISOLATED_WORLD_ID_BRAVE_PSST);
  }

  // The native half of the flow logs to the same timeline while navigating, so
  // these tests only look at what the renderer sent.
  std::vector<mojom::PsstLogEntryPtr> ScriptEntries() {
    std::vector<mojom::PsstLogEntryPtr> script_entries;
    for (auto& entry : log_service()->GetEntries()) {
      if (entry->source == mojom::PsstLogSource::kScript) {
        script_entries.push_back(std::move(entry));
      }
    }
    return script_entries;
  }

  // Polls until the service holds at least `count` script entries. Returns them
  // so callers can assert on them.
  std::vector<mojom::PsstLogEntryPtr> WaitForScriptEntries(
      size_t count,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    if (!base::test::RunUntil(
            [&]() { return ScriptEntries().size() >= count; })) {
      ADD_FAILURE() << "Timed out waiting for " << count
                    << " script entries; have " << ScriptEntries().size();
    }
    return ScriptEntries();
  }

  net::EmbeddedTestServer https_server_;

 private:
  base::test::ScopedFeatureList feature_list_;
};

// The page must not be able to see or spoof the logging function.
IN_PROC_BROWSER_TEST_F(PsstLogReceiverBrowserTest, NotExposedToPageScript) {
  ASSERT_NO_FATAL_FAILURE(NavigateToPageWithIframe(kMainFrameHost));

  EXPECT_EQ("undefined", content::EvalJs(web_contents(), "typeof psstLog"));
  EXPECT_EQ("undefined",
            content::EvalJs(web_contents(),
                            base::StrCat({"typeof ", kPsstNativeObjectName})));
}

IN_PROC_BROWSER_TEST_F(PsstLogReceiverBrowserTest,
                       ExposedInThePsstIsolatedWorld) {
  ASSERT_NO_FATAL_FAILURE(NavigateToPageWithIframe(kMainFrameHost));

  EXPECT_EQ("function", EvalInPsstWorld(web_contents(), "typeof psstLog"));
  // Defined non-configurably, so a script sharing the world can't replace it.
  EXPECT_EQ(false, EvalInPsstWorld(web_contents(),
                                   "Object.getOwnPropertyDescriptor("
                                   "globalThis, 'psstLog').configurable"));
}

IN_PROC_BROWSER_TEST_F(PsstLogReceiverBrowserTest, EntryIsStampedByTheBrowser) {
  ASSERT_NO_FATAL_FAILURE(NavigateToPageWithIframe(kMainFrameHost));
  ASSERT_TRUE(log_service());

  ASSERT_TRUE(EvalInPsstWorld(web_contents(),
                              "psstLog('warn', 'hello', { count: 2 }), true")
                  .ExtractBool());

  const auto entries = WaitForScriptEntries(1);
  ASSERT_EQ(1u, entries.size());
  EXPECT_EQ(mojom::PsstLogSource::kScript, entries[0]->source);
  EXPECT_EQ(mojom::PsstLogLevel::kWarn, entries[0]->level);
  EXPECT_EQ("hello", entries[0]->message);
  ASSERT_TRUE(entries[0]->data.has_value());
  EXPECT_EQ(base::test::ParseJsonDict(R"({"count":2})"),
            base::test::ParseJsonDict(*entries[0]->data));
  EXPECT_EQ(kMainFrameHost, entries[0]->site);
  EXPECT_EQ(sessions::SessionTabHelper::IdForTab(web_contents()).id(),
            entries[0]->tab_id);
  EXPECT_EQ(web_contents()->GetPrimaryMainFrame()->GetFrameToken().ToString(),
            entries[0]->frame_token);
}

// An unrecognized level is still recorded, as info.
IN_PROC_BROWSER_TEST_F(PsstLogReceiverBrowserTest, UnknownLevelBecomesInfo) {
  ASSERT_NO_FATAL_FAILURE(NavigateToPageWithIframe(kMainFrameHost));

  ASSERT_TRUE(EvalInPsstWorld(web_contents(), "psstLog('shout', 'hello'), true")
                  .ExtractBool());

  const auto entries = WaitForScriptEntries(1);
  ASSERT_EQ(1u, entries.size());
  EXPECT_EQ(mojom::PsstLogLevel::kInfo, entries[0]->level);
}

// Scripts are untrusted input: an oversized message is truncated rather than
// allowed to dominate the buffer.
IN_PROC_BROWSER_TEST_F(PsstLogReceiverBrowserTest, OverlongMessageIsTruncated) {
  ASSERT_NO_FATAL_FAILURE(NavigateToPageWithIframe(kMainFrameHost));

  ASSERT_TRUE(
      EvalInPsstWorld(web_contents(), "psstLog('info', 'x'.repeat(5000)), true")
          .ExtractBool());

  const auto entries = WaitForScriptEntries(1);
  ASSERT_EQ(1u, entries.size());
  EXPECT_EQ(PsstLogReceiver::kMaxScriptMessageLength,
            entries[0]->message.size());
}

// Anything that isn't a plain object is dropped rather than recorded.
IN_PROC_BROWSER_TEST_F(PsstLogReceiverBrowserTest, NonObjectDataIsIgnored) {
  ASSERT_NO_FATAL_FAILURE(NavigateToPageWithIframe(kMainFrameHost));

  ASSERT_TRUE(
      EvalInPsstWorld(web_contents(), "psstLog('info', 'm', 'nope'), true")
          .ExtractBool());

  const auto entries = WaitForScriptEntries(1);
  ASSERT_EQ(1u, entries.size());
  EXPECT_EQ("m", entries[0]->message);
  EXPECT_FALSE(entries[0]->data.has_value());
}

// A call with no message throws, and records nothing.
IN_PROC_BROWSER_TEST_F(PsstLogReceiverBrowserTest, MissingMessageThrows) {
  ASSERT_NO_FATAL_FAILURE(NavigateToPageWithIframe(kMainFrameHost));

  // `psstLog` swallows the native TypeError so logging can never break the
  // flow, so observe it by calling the native object directly.
  EXPECT_FALSE(
      EvalInPsstWorld(web_contents(),
                      base::StrCat({"(() => { try { ", kPsstNativeObjectName,
                                    ".psstLog('info'); return true; } "
                                    "catch (e) { return false; } })()"}))
          .ExtractBool());
  EXPECT_TRUE(ScriptEntries().empty());
}

IN_PROC_BROWSER_TEST_F(PsstLogReceiverBrowserTest, ConsoleIsForwarded) {
  ASSERT_NO_FATAL_FAILURE(NavigateToPageWithIframe(kMainFrameHost));

  ASSERT_TRUE(
      EvalInPsstWorld(web_contents(), "console.warn('from', 'console'), true")
          .ExtractBool());

  const auto entries = WaitForScriptEntries(1);
  ASSERT_EQ(1u, entries.size());
  EXPECT_EQ(mojom::PsstLogSource::kScript, entries[0]->source);
  EXPECT_EQ(mojom::PsstLogLevel::kWarn, entries[0]->level);
  EXPECT_EQ("from console", entries[0]->message);
}

// A subframe's entries are attributed to that subframe's site and frame, and
// still grouped under the tab the subframe lives in.
IN_PROC_BROWSER_TEST_F(PsstLogReceiverBrowserTest,
                       SubframeIsAttributedToItself) {
  ASSERT_NO_FATAL_FAILURE(NavigateToPageWithIframe(kMainFrameHost));

  content::RenderFrameHost* subframe = content::ChildFrameAt(web_contents(), 0);
  ASSERT_TRUE(subframe);
  ASSERT_TRUE(content::NavigateToURLFromRenderer(
      subframe, https_server_.GetURL(kSubFrameHost, "/simple.html")));
  subframe = content::ChildFrameAt(web_contents(), 0);
  ASSERT_TRUE(subframe);

  ASSERT_TRUE(
      EvalInPsstWorld(subframe, "psstLog('info', 'from subframe'), true")
          .ExtractBool());

  const auto entries = WaitForScriptEntries(1);
  ASSERT_EQ(1u, entries.size());
  EXPECT_EQ("from subframe", entries[0]->message);
  EXPECT_EQ(kSubFrameHost, entries[0]->site);
  EXPECT_EQ(sessions::SessionTabHelper::IdForTab(web_contents()).id(),
            entries[0]->tab_id);
  EXPECT_EQ(subframe->GetFrameToken().ToString(), entries[0]->frame_token);
}

}  // namespace psst
