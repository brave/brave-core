/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <set>
#include <string>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/synchronization/lock.h"
#include "base/test/scoped_feature_list.h"
#include "base/thread_annotations.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "third_party/blink/public/common/features.h"

namespace {

constexpr char kCspReportUriPage[] = "/csp_report_uri.html";
constexpr char kCspReportPath[] = "/csp-report";
constexpr char kSentinelPath[] = "/sentinel";

}  // namespace

class ReportingApiBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    embedded_test_server()->ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
        &ReportingApiBrowserTest::OnRequest, base::Unretained(this)));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 protected:
  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void WaitForRequest(const std::string& path) {
    base::RunLoop run_loop;
    {
      base::AutoLock lock(lock_);
      if (seen_paths_.contains(path)) {
        return;
      }
      awaited_path_ = path;
      on_awaited_path_ = run_loop.QuitClosure();
    }
    run_loop.Run();
  }

  bool HasSeenRequest(const std::string& path) {
    base::AutoLock lock(lock_);
    return seen_paths_.contains(path);
  }

  void TriggerCspViolation() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL(kCspReportUriPage)));
    EXPECT_EQ(true, content::EvalJs(web_contents(), "triggerViolation()"));
  }

 private:
  // Runs on the test server's IO thread.
  void OnRequest(const net::test_server::HttpRequest& request) {
    base::AutoLock lock(lock_);
    const std::string path(request.GetURL().path());
    seen_paths_.insert(path);
    if (path == awaited_path_ && on_awaited_path_) {
      std::move(on_awaited_path_).Run();
    }
  }

  base::Lock lock_;
  std::set<std::string> seen_paths_ GUARDED_BY(lock_);
  std::string awaited_path_ GUARDED_BY(lock_);
  base::OnceClosure on_awaited_path_ GUARDED_BY(lock_);
};

IN_PROC_BROWSER_TEST_F(ReportingApiBrowserTest, CspReportUriNotSent) {
  TriggerCspViolation();

  // A report would be sent before this request, so its arrival bounds the wait.
  EXPECT_TRUE(content::ExecJs(web_contents(),
                              content::JsReplace("fetch($1)", kSentinelPath)));
  WaitForRequest(kSentinelPath);
  EXPECT_FALSE(HasSeenRequest(kCspReportPath));
}

class ReportingApiEnabledBrowserTest : public ReportingApiBrowserTest {
 public:
  ReportingApiEnabledBrowserTest() {
    feature_list_.InitAndEnableFeature(blink::features::kBraveReportingAPI);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(ReportingApiEnabledBrowserTest, CspReportUriSent) {
  TriggerCspViolation();
  WaitForRequest(kCspReportPath);
}
