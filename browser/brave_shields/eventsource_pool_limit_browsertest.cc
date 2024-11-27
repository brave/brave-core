/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <string_view>

#include "base/path_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/webcompat/core/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "extensions/buildflags/buildflags.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_connection.h"
#include "net/test/embedded_test_server/http_request.h"
#include "third_party/blink/public/common/features.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "extensions/common/extension.h"
#include "extensions/test/test_extension_dir.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

namespace {

using net::test_server::BasicHttpResponse;
using net::test_server::EmbeddedTestServer;
using net::test_server::EmbeddedTestServerConnectionListener;
using net::test_server::HttpConnection;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;

constexpr int kEventSourcesPoolLimit = 250;

constexpr char kEventSourcesOpenScript[] = R"(
  if (typeof sources === "undefined") {
    sources = [];
  }
  new Promise(resolve => {
    const source = new EventSource($1);
    sources.push(source);
    source.addEventListener('open', () => {
      resolve('open');
    });
    source.addEventListener('error', () => {
      resolve('error');
    });
  });
)";

constexpr char kEventSourceCloseScript[] = R"(
  sources[$1].close();
)";

constexpr char kRegisterSwScript[] = R"(
  (async () => {
    await navigator.serviceWorker.register($1, {scope: './'});
    const registration = await navigator.serviceWorker.ready;
  })();
)";

constexpr char kEventSourcesOpenInSwScript[] = R"(
  (async () => {
    const registration = await navigator.serviceWorker.ready;
    const result = new Promise(resolve => {
      navigator.serviceWorker.onmessage = event => {
        resolve(event.data);
      };
    });
    registration.active.postMessage({cmd: 'open_es', url: $1});
    return await result;
  })();
)";

constexpr char kEventSourceCloseInSwScript[] = R"(
  (async () => {
    const registration = await navigator.serviceWorker.ready;
    registration.active.postMessage({cmd: 'close_es', idx: $1});
  })();
)";

}  // namespace

class EventSourcePoolLimitBrowserTest : public InProcessBrowserTest {
 public:
  EventSourcePoolLimitBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        webcompat::features::kBraveWebcompatExceptionsService);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    content::SetupCrossSiteRedirector(&https_server_);
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&EventSourcePoolLimitBrowserTest::HandleRequest,
                            base::Unretained(this)));
    ASSERT_TRUE(https_server_.Start());
    es_url_ = https_server_.GetURL("a.com", "/source");
  }

  std::unique_ptr<HttpResponse> HandleRequest(const HttpRequest& request) {
    std::string event_source_host_colon_port =
        es_url_.host() + ":" + es_url_.port();
    if (request.relative_url == es_url_.path() &&
        request.headers.at("Host") == event_source_host_colon_port) {
      auto http_response = std::make_unique<BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      http_response->set_content_type("text/event-stream");
      http_response->AddCustomHeader("Access-Control-Allow-Origin", "*");
      http_response->AddCustomHeader("Cache-Control", "no-cache");
      http_response->AddCustomHeader("Connection", "keep-alive");
      http_response->set_content("retry: 10000\n\n");
      return http_response;
    }
    return nullptr;
  }

  void OpenEventSources(content::RenderFrameHost* rfh,
                        std::string_view script_template,
                        int count) {
    const std::string& es_open_script =
        content::JsReplace(script_template, es_url_);
    for (int i = 0; i < count; ++i) {
      EXPECT_EQ("open", content::EvalJs(rfh, es_open_script));
    }
  }

  void ExpectEventSourcesAreLimited(content::RenderFrameHost* rfh,
                                    std::string_view script_template) {
    const std::string& es_open_script =
        content::JsReplace(script_template, es_url_);
    for (int i = 0; i < 5; ++i) {
      EXPECT_EQ("error", content::EvalJs(rfh, es_open_script));
    }
  }

  void CloseEventSources(content::RenderFrameHost* rfh,
                         std::string_view script_template,
                         int count) {
    for (int i = 0; i < count; ++i) {
      EXPECT_TRUE(content::ExecJs(rfh, content::JsReplace(script_template, i)));
    }
  }

  void OpenEventSourcesAndExpectLimited(content::RenderFrameHost* rfh,
                                        std::string_view script_template,
                                        int count) {
    OpenEventSources(rfh, script_template, count);
    ExpectEventSourcesAreLimited(rfh, script_template);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  // Makes use of Cross Site Redirector
  content::RenderFrameHost* GetNthChildFrameWithHost(
      content::RenderFrameHost* main,
      std::string_view host,
      size_t n = 0) {
    size_t child_idx = 0;
    while (true) {
      auto* child_rfh = content::ChildFrameAt(main, child_idx++);
      if (!child_rfh) {
        return nullptr;
      }
      if (child_rfh->GetLastCommittedOrigin().host() == host) {
        if (!n) {
          return child_rfh;
        }
        --n;
      }
    }
  }

 protected:
  content::ContentMockCertVerifier mock_cert_verifier_;
  EmbeddedTestServer https_server_{EmbeddedTestServer::TYPE_HTTPS};
  GURL es_url_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(EventSourcePoolLimitBrowserTest,
                       PoolIsLimitedByDefault) {
  const GURL url(https_server_.GetURL("a.com", "/simple.html"));
  auto* rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  OpenEventSourcesAndExpectLimited(rfh, kEventSourcesOpenScript,
                                   kEventSourcesPoolLimit);
  CloseEventSources(rfh, kEventSourceCloseScript, 5);
  OpenEventSourcesAndExpectLimited(rfh, kEventSourcesOpenScript, 5);
}

IN_PROC_BROWSER_TEST_F(EventSourcePoolLimitBrowserTest,
                       PoolIsKeyedByTopFrameOrigin) {
  const GURL a_com_url(
      https_server_.GetURL("a.com", "/ephemeral_storage.html"));
  const GURL b_com_url(
      https_server_.GetURL("b.com", "/ephemeral_storage.html"));

  // Open a.com with nested b.com.
  auto* a_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), a_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  auto* b_com0_in_a_com_rfh = GetNthChildFrameWithHost(a_com_rfh, "b.com");

  // Test EventSource limit in nested b.com.
  OpenEventSourcesAndExpectLimited(b_com0_in_a_com_rfh, kEventSourcesOpenScript,
                                   kEventSourcesPoolLimit);

  // Expect the limit is also active in another nested b.com.
  auto* b_com1_in_a_com_rfh = GetNthChildFrameWithHost(a_com_rfh, "b.com", 1);
  ExpectEventSourcesAreLimited(b_com1_in_a_com_rfh, kEventSourcesOpenScript);

  // Expect the limit is NOT active in the first-party a.com frame, bc the pool
  // is located in the a.com renderer process.
  // TODO(aedelstein@brave.com): Check why -- possible concern?
  auto* a_com_in_a_com_rfh = GetNthChildFrameWithHost(a_com_rfh, "a.com");
  OpenEventSources(a_com_in_a_com_rfh, kEventSourcesOpenScript, 1);

  // Open b.com with a nested a.com.
  auto* b_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), b_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  auto* a_com_in_b_com_rfh = GetNthChildFrameWithHost(b_com_rfh, "a.com");

  // Test EventSources limit in nested a.com.
  OpenEventSourcesAndExpectLimited(a_com_in_b_com_rfh, kEventSourcesOpenScript,
                                   kEventSourcesPoolLimit);

  // Expect the limit is STILL NOT active in the first-party a.com frame.
  OpenEventSources(a_com_in_a_com_rfh, kEventSourcesOpenScript, 1);
}

IN_PROC_BROWSER_TEST_F(EventSourcePoolLimitBrowserTest,
                       ServiceWorkerIsLimited) {
  const GURL url(https_server_.GetURL("a.com", "/simple.html"));

  auto* rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  const std::string& register_sw_script = content::JsReplace(
      kRegisterSwScript, "service-worker-eventsource-limit.js");
  ASSERT_TRUE(content::ExecJs(rfh, register_sw_script));

  OpenEventSourcesAndExpectLimited(rfh, kEventSourcesOpenInSwScript,
                                   kEventSourcesPoolLimit);
  CloseEventSources(rfh, kEventSourceCloseInSwScript, 5);
  OpenEventSources(rfh, kEventSourcesOpenInSwScript, 5);
  ExpectEventSourcesAreLimited(rfh, kEventSourcesOpenInSwScript);
  // Expect no Event Sources can be created on a webpage when a limit is hit.
  ExpectEventSourcesAreLimited(rfh, kEventSourcesOpenScript);
}

// Ensures that sub-frame opaque origins are treated properly when used from
// different top-frame opaque origins.
// TODO(https://github.com/brave/brave-browser/issues/28393): Test flaky on
// master for the windows asan build.
#if BUILDFLAG(IS_WIN) && defined(ADDRESS_SANITIZER)
#define MAYBE_SandboxedFramesAreLimited DISABLED_SandboxedFramesAreLimited
#else
#define MAYBE_SandboxedFramesAreLimited SandboxedFramesAreLimited
#endif  // BUILDFLAG(IS_MAC)
IN_PROC_BROWSER_TEST_F(EventSourcePoolLimitBrowserTest,
                       MAYBE_SandboxedFramesAreLimited) {
  const GURL a_com_url(
      https_server_.GetURL("a.com", "/csp_sandboxed_frame.html"));
  auto* a_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), a_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  EXPECT_TRUE(a_com_rfh->GetLastCommittedOrigin().opaque());

  // Ensure the limit is applied to main a.com and child c.com frames.
  OpenEventSourcesAndExpectLimited(a_com_rfh, kEventSourcesOpenScript,
                                   kEventSourcesPoolLimit);
  OpenEventSourcesAndExpectLimited(content::ChildFrameAt(a_com_rfh, 0),
                                   kEventSourcesOpenScript,
                                   kEventSourcesPoolLimit);

  const GURL b_com_url(
      https_server_.GetURL("b.com", "/csp_sandboxed_frame.html"));
  auto* b_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), b_com_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  EXPECT_TRUE(b_com_rfh->GetLastCommittedOrigin().opaque());

  // Ensure the limit is applied to main b.com and child c.com frames.
  OpenEventSourcesAndExpectLimited(b_com_rfh, kEventSourcesOpenScript,
                                   kEventSourcesPoolLimit);
  OpenEventSourcesAndExpectLimited(content::ChildFrameAt(b_com_rfh, 0),
                                   kEventSourcesOpenScript,
                                   kEventSourcesPoolLimit);
}

IN_PROC_BROWSER_TEST_F(EventSourcePoolLimitBrowserTest,
                       PoolIsNotLimitedWithDisabledShields) {
  const GURL url(https_server_.GetURL("a.com", "/ephemeral_storage.html"));
  // Disable shields.
  brave_shields::SetBraveShieldsEnabled(content_settings(), false, url);

  auto* a_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // No limits should be active.
  OpenEventSources(a_com_rfh, kEventSourcesOpenScript,
                   kEventSourcesPoolLimit + 5);

  // No limits should be active in a 3p frame.
  auto* b_com_in_a_com_rfh = GetNthChildFrameWithHost(a_com_rfh, "b.com");
  OpenEventSources(b_com_in_a_com_rfh, kEventSourcesOpenScript,
                   kEventSourcesPoolLimit + 5);

  // No limits should be active in a ServiceWorker.
  const std::string& register_sw_script = content::JsReplace(
      kRegisterSwScript, "service-worker-eventsource-limit.js");
  ASSERT_TRUE(content::ExecJs(a_com_rfh, register_sw_script));
  OpenEventSources(a_com_rfh, kEventSourcesOpenInSwScript,
                   kEventSourcesPoolLimit + 5);
}

IN_PROC_BROWSER_TEST_F(EventSourcePoolLimitBrowserTest,
                       PoolIsNotLimitedWithWebcompatException) {
  const GURL url(https_server_.GetURL("a.com", "/ephemeral_storage.html"));

  // Enable shields.
  brave_shields::SetBraveShieldsEnabled(content_settings(), true, url);

  // Enable webcompat exception.
  brave_shields::SetWebcompatEnabled(
      content_settings(),
      ContentSettingsType::BRAVE_WEBCOMPAT_EVENT_SOURCE_POOL, true,
      https_server_.GetURL("a.com", "/"), nullptr);

  auto* a_com_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // No limits should be active.
  OpenEventSources(a_com_rfh, kEventSourcesOpenScript,
                   kEventSourcesPoolLimit + 5);

  // No limits should be active in a 3p frame.
  auto* b_com_in_a_com_rfh = GetNthChildFrameWithHost(a_com_rfh, "b.com");
  OpenEventSources(b_com_in_a_com_rfh, kEventSourcesOpenScript,
                   kEventSourcesPoolLimit + 5);
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
IN_PROC_BROWSER_TEST_F(EventSourcePoolLimitBrowserTest,
                       PoolIsNotLimitedForExtensions) {
  extensions::TestExtensionDir test_extension_dir;
  test_extension_dir.WriteManifest(R"({
    "name": "Test",
    "manifest_version": 2,
    "version": "0.1",
    "permissions": ["webRequest", "webRequestBlocking", "*://a.com/*"],
    "content_security_policy":
      "script-src 'self' 'unsafe-eval'; object-src 'self'"
  })");
  test_extension_dir.WriteFile(FILE_PATH_LITERAL("empty.html"), "");

  extensions::ChromeTestExtensionLoader extension_loader(browser()->profile());
  scoped_refptr<const extensions::Extension> extension =
      extension_loader.LoadExtension(test_extension_dir.UnpackedPath());
  const GURL url = extension->GetResourceURL("/empty.html");
  auto* extension_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(extension_rfh);
  OpenEventSources(extension_rfh, kEventSourcesOpenScript,
                   kEventSourcesPoolLimit + 5);
}
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

class EventSourcePoolLimitDisabledBrowserTest
    : public EventSourcePoolLimitBrowserTest {
 public:
  EventSourcePoolLimitDisabledBrowserTest() {
    scoped_feature_list_.InitAndDisableFeature(
        blink::features::kRestrictEventSourcePool);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(EventSourcePoolLimitDisabledBrowserTest,
                       PoolIsNotLimited) {
  const GURL url(https_server_.GetURL("a.com", "/simple.html"));
  auto* rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // No limits should be active.
  OpenEventSources(rfh, kEventSourcesOpenScript, kEventSourcesPoolLimit + 5);
}
