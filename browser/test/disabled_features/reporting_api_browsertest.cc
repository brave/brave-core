/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <set>
#include <string>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/synchronization/lock.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "base/thread_annotations.h"
#include "base/time/time.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/embedder_support/switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "third_party/blink/public/common/features.h"

namespace {

constexpr char kCspReportUriPage[] = "/csp_report_uri.html";
constexpr char kCspReportPath[] = "/csp-report";
constexpr char kSentinelPath[] = "/sentinel";

constexpr char kCspReportToPage[] = "/csp_report_to.html";
constexpr char kReportToUploadPath[] = "/report-to-upload";

// Many multiples of the 100ms delivery interval set by --short-reporting-delay.
constexpr base::TimeDelta kReportToUploadTimeout = base::Seconds(2);

}  // namespace

class ReportingApiBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(embedder_support::kShortReportingDelay);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    https_server_.ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &ReportingApiBrowserTest::OnRequest, base::Unretained(this)));
    ASSERT_TRUE(https_server_.Start());
  }

 protected:
  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  void WaitForRequest(const std::string& path) {
    base::RunLoop run_loop;
    if (SetAwaitedPath(path, run_loop.QuitClosure())) {
      run_loop.Run();
    }
  }

  bool WaitForRequestWithTimeout(const std::string& path,
                                 base::TimeDelta timeout) {
    base::RunLoop run_loop;
    if (SetAwaitedPath(path, run_loop.QuitClosure())) {
      base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
          FROM_HERE, run_loop.QuitClosure(), timeout);
      run_loop.Run();
    }
    return HasSeenRequest(path);
  }

  bool HasSeenRequest(const std::string& path) {
    base::AutoLock lock(lock_);
    return seen_paths_.contains(path);
  }

  void TriggerCspViolation(const std::string& page) {
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), https_server_.GetURL(page)));
    EXPECT_EQ(true, content::EvalJs(web_contents(), "triggerViolation()"));
  }

  void AllowBackgroundSync() {
    const GURL url = https_server_.GetURL("/");
    HostContentSettingsMapFactory::GetForProfile(
        chrome_test_utils::GetProfile(this))
        ->SetContentSettingDefaultScope(url, url,
                                        ContentSettingsType::BACKGROUND_SYNC,
                                        CONTENT_SETTING_ALLOW);
  }

 private:
  // Returns false if `path` was already seen, so there is nothing to wait for.
  bool SetAwaitedPath(const std::string& path, base::OnceClosure on_seen) {
    base::AutoLock lock(lock_);
    if (seen_paths_.contains(path)) {
      return false;
    }
    awaited_path_ = path;
    on_awaited_path_ = std::move(on_seen);
    return true;
  }

  // Runs on the test server's IO thread.
  void OnRequest(const net::test_server::HttpRequest& request) {
    base::AutoLock lock(lock_);
    const std::string path(request.GetURL().path());
    seen_paths_.insert(path);
    if (path == awaited_path_ && on_awaited_path_) {
      std::move(on_awaited_path_).Run();
    }
  }

  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
  base::Lock lock_;
  std::set<std::string> seen_paths_ GUARDED_BY(lock_);
  std::string awaited_path_ GUARDED_BY(lock_);
  base::OnceClosure on_awaited_path_ GUARDED_BY(lock_);
};

IN_PROC_BROWSER_TEST_F(ReportingApiBrowserTest, CspReportUriNotSent) {
  TriggerCspViolation(kCspReportUriPage);

  // A report would be sent before this request, so its arrival bounds the wait.
  EXPECT_TRUE(content::ExecJs(web_contents(),
                              content::JsReplace("fetch($1)", kSentinelPath)));
  WaitForRequest(kSentinelPath);
  EXPECT_FALSE(HasSeenRequest(kCspReportPath));
}

IN_PROC_BROWSER_TEST_F(ReportingApiBrowserTest,
                       ReportToNotSentEvenWithBackgroundSync) {
  AllowBackgroundSync();
  TriggerCspViolation(kCspReportToPage);
  EXPECT_FALSE(
      WaitForRequestWithTimeout(kReportToUploadPath, kReportToUploadTimeout));
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
  TriggerCspViolation(kCspReportUriPage);
  WaitForRequest(kCspReportPath);
}

IN_PROC_BROWSER_TEST_F(ReportingApiEnabledBrowserTest,
                       ReportToSentWithoutBackgroundSync) {
  TriggerCspViolation(kCspReportToPage);
  WaitForRequest(kReportToUploadPath);
}
