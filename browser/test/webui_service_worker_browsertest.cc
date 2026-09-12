/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted_memory.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/test_future.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/service_worker_context_observer.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/browser/webui_config_map.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/public/test/web_ui_browsertest_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/service_worker/service_worker_status_code.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration_options.mojom.h"
#include "ui/webui/untrusted_web_ui_browsertest_util.h"
#include "ui/webui/untrusted_web_ui_controller.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

// Distinct bodies, so that a page can be attributed to whichever of the two
// possible sources served it.
constexpr char kDataSourceBody[] = "from-data-source";
constexpr char kServiceWorkerBody[] = "from-service-worker";

// A path the service worker declines to handle, which is how a request reaches
// the WebUI loader factory that the navigation shortcut used to be the only
// source of.
constexpr char kUnhandledPath[] = "unhandled";

constexpr char kScriptPath[] = "sw.js";

// A host whose config opts in to being reachable by a service worker, and one
// that leaves the default alone.
constexpr char kInterceptedHost[] = "sw-webui";
constexpr char kNotInterceptedHost[] = "no-sw-webui";

constexpr char kServiceWorkerScriptTemplate[] = R"(
  self.oninstall = e => e.waitUntil(self.skipWaiting());
  self.onactivate = e => e.waitUntil(self.clients.claim());
  self.onfetch = e => {
    if (new URL(e.request.url).pathname === '/' + $1) {
      return;  // No respondWith(): the request is left to the WebUI factory.
    }
    e.respondWith(
        new Response($2, {headers: {'content-type': 'text/html'}}));
  };
)";

GURL GetUntrustedURL(std::string_view host_and_path) {
  return content::GetChromeUntrustedUIURL(std::string(host_and_path));
}

blink::StorageKey GetStorageKey(const GURL& url) {
  return blink::StorageKey::CreateFirstParty(url::Origin::Create(url));
}

// Serves the service worker script, and for every other path a page whose body
// names this data source as what served it.
class StaticDataSource : public content::URLDataSource {
 public:
  explicit StaticDataSource(std::string source) : source_(std::move(source)) {}
  ~StaticDataSource() override = default;

  // content::URLDataSource:
  std::string GetSource() override { return source_; }

  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override {
    std::string body =
        IsScript(url) ? content::JsReplace(kServiceWorkerScriptTemplate,
                                           kUnhandledPath, kServiceWorkerBody)
                      : kDataSourceBody;
    std::move(callback).Run(
        base::MakeRefCounted<base::RefCountedString>(std::move(body)));
  }

  std::string GetMimeType(const GURL& url) override {
    return IsScript(url) ? "text/javascript" : "text/html";
  }

  // The pages served here are plain text, and a CSP would only get in the way
  // of reading them back.
  bool ShouldAddContentSecurityPolicy() override { return false; }

 private:
  static bool IsScript(const GURL& url) {
    return url.ExtractFileName() == kScriptPath;
  }

  const std::string source_;
};

class StaticWebUIConfig : public content::WebUIConfig {
 public:
  StaticWebUIConfig(std::string_view scheme,
                    std::string_view host,
                    bool intercept_navigations)
      : content::WebUIConfig(scheme, host),
        intercept_navigations_(intercept_navigations) {}
  ~StaticWebUIConfig() override = default;

  // content::WebUIConfig:
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override {
    content::BrowserContext* browser_context =
        web_ui->GetWebContents()->GetBrowserContext();
    RegisterURLDataSource(browser_context);
    return std::make_unique<ui::UntrustedWebUIController>(web_ui);
  }

  void RegisterURLDataSource(
      content::BrowserContext* browser_context) override {
    content::URLDataSource::Add(
        browser_context,
        std::make_unique<StaticDataSource>(GetDataSourceName()));
  }

  bool ShouldInterceptNavigationsWithServiceWorker() override {
    return intercept_navigations_;
  }

 private:
  // chrome-untrusted:// data sources are keyed by origin, chrome:// ones by
  // host.
  std::string GetDataSourceName() const {
    return scheme() == content::kChromeUIUntrustedScheme
               ? GetUntrustedURL(host()).spec()
               : host();
  }

  const bool intercept_navigations_;
};

// Waits for a service worker to reach its activated state, which is when it can
// serve a navigation.
class ActivationObserver : public content::ServiceWorkerContextObserver {
 public:
  explicit ActivationObserver(content::ServiceWorkerContext* context)
      : context_(context) {
    context_->AddObserver(this);
  }

  ~ActivationObserver() override { context_->RemoveObserver(this); }

  void Wait() { run_loop_.Run(); }

 private:
  // content::ServiceWorkerContextObserver:
  void OnVersionActivated(int64_t version_id, const GURL& scope) override {
    run_loop_.Quit();
  }

  const raw_ptr<content::ServiceWorkerContext> context_;
  base::RunLoop run_loop_;
};

}  // namespace

class WebUIServiceWorkerBrowserTest : public InProcessBrowserTest {
 protected:
  // Registers a config for `base_url`'s scheme and host, which serves the
  // service worker script for that host, and either opts in to service worker
  // navigation interception or leaves it at its default.
  void AddWebUIConfig(const GURL& base_url, bool intercept_navigations) {
    auto config = std::make_unique<StaticWebUIConfig>(
        base_url.GetScheme(), base_url.GetHost(), intercept_navigations);
    auto& config_map = content::WebUIConfigMap::GetInstance();
    if (base_url.SchemeIs(content::kChromeUIUntrustedScheme)) {
      config_map.AddUntrustedWebUIConfig(std::move(config));
    } else {
      config_map.AddWebUIConfig(std::move(config));
    }
  }

  content::ServiceWorkerContext* service_worker_context() {
    return browser()
        ->GetProfile()
        ->GetDefaultStoragePartition()
        ->GetServiceWorkerContext();
  }

  // Registers a service worker for `scope` the only way a WebUI can: from C++.
  blink::ServiceWorkerStatusCode RegisterServiceWorker(const GURL& scope) {
    blink::mojom::ServiceWorkerRegistrationOptions options(
        scope, blink::mojom::ScriptType::kClassic,
        blink::mojom::ServiceWorkerUpdateViaCache::kNone);
    base::test::TestFuture<blink::ServiceWorkerStatusCode> future;
    service_worker_context()->RegisterServiceWorker(
        scope.Resolve(kScriptPath), GetStorageKey(scope), options,
        content::GlobalRenderFrameHostId(), future.GetCallback());
    return future.Get();
  }

  // As above, but also waits for the worker to be able to serve requests.
  [[nodiscard]] bool RegisterAndActivateServiceWorker(const GURL& scope) {
    ActivationObserver observer(service_worker_context());
    if (RegisterServiceWorker(scope) != blink::ServiceWorkerStatusCode::kOk) {
      return false;
    }
    observer.Wait();
    return true;
  }

  blink::ServiceWorkerStatusCode UnregisterServiceWorker(const GURL& scope) {
    base::test::TestFuture<blink::ServiceWorkerStatusCode> future;
    service_worker_context()->UnregisterServiceWorkerImmediately(
        scope, GetStorageKey(scope), future.GetCallback());
    return future.Get();
  }

  content::ServiceWorkerCapability GetServiceWorkerCapability(
      const GURL& scope) {
    base::test::TestFuture<content::ServiceWorkerCapability> future;
    service_worker_context()->CheckHasServiceWorker(scope, GetStorageKey(scope),
                                                    future.GetCallback());
    return future.Get();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  // The body of the page at `url`, which names whatever served it.
  content::EvalJsResult NavigateAndGetBody(const GURL& url) {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    EXPECT_EQ(url, web_contents()->GetLastCommittedURL());
    // The navigation above can return before the document has been parsed, so
    // wait for the body that is being asked for to exist.
    return content::EvalJs(web_contents(), R"(
        new Promise(resolve => {
          if (document.readyState === 'loading') {
            document.addEventListener(
                'DOMContentLoaded', () => resolve(document.body.innerText));
            return;
          }
          resolve(document.body.innerText);
        })
    )");
  }

  // Whether the document has a service worker client with a controller, which
  // is what makes its subresources reachable by the worker too.
  content::EvalJsResult IsControlled() {
    return content::EvalJs(web_contents(),
                           "navigator.serviceWorker.controller !== null");
  }
};

// A WebUI service worker's script is fetched from the data source its config
// registers, so a host with a config behind it can be given a worker.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest, RegistersAndUnregisters) {
  const GURL scope = GetUntrustedURL(kInterceptedHost);
  AddWebUIConfig(scope, /*intercept_navigations=*/true);

  EXPECT_EQ(blink::ServiceWorkerStatusCode::kOk, RegisterServiceWorker(scope));
  EXPECT_EQ(content::ServiceWorkerCapability::SERVICE_WORKER_WITH_FETCH_HANDLER,
            GetServiceWorkerCapability(scope));

  EXPECT_EQ(blink::ServiceWorkerStatusCode::kOk,
            UnregisterServiceWorker(scope));
  EXPECT_EQ(content::ServiceWorkerCapability::NO_SERVICE_WORKER,
            GetServiceWorkerCapability(scope));
}

// Registration is not gated on the navigation opt-in: a worker can serve a
// host's subresources without serving its navigations.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest,
                       RegistersWithoutNavigationOptIn) {
  const GURL scope = GetUntrustedURL(kNotInterceptedHost);
  AddWebUIConfig(scope, /*intercept_navigations=*/false);

  EXPECT_EQ(blink::ServiceWorkerStatusCode::kOk, RegisterServiceWorker(scope));
}

// Without a config there is no data source to fetch the script from, and
// nothing legitimate to serve, so registration fails.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest,
                       RegistrationFailsWithoutWebUIConfig) {
  EXPECT_EQ(blink::ServiceWorkerStatusCode::kErrorNetwork,
            RegisterServiceWorker(GetUntrustedURL("no-config")));
}

// Only chrome-untrusted:// gained a script factory, so a chrome:// host cannot
// be given a worker, even if its config asks for interception.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest,
                       RegistrationFailsForChromeScheme) {
  const GURL scope = content::GetWebUIURL("sw-trusted-webui");
  AddWebUIConfig(scope, /*intercept_navigations=*/true);

  EXPECT_EQ(blink::ServiceWorkerStatusCode::kErrorNetwork,
            RegisterServiceWorker(scope));
}

// A page cannot give itself a worker, which is what keeps registration a
// browser-side decision.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest,
                       RegistrationFromJavaScriptIsDisallowed) {
  const GURL base_url = GetUntrustedURL(kInterceptedHost);
  AddWebUIConfig(base_url, /*intercept_navigations=*/true);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), base_url));

  const std::string unsupported_origin = base::StrCat(
      {"The URL protocol of the current origin ('",
       url::Origin::Create(base_url).Serialize(), "') is not supported."});

  const std::string register_script = content::JsReplace(R"(
      (async () => {
        try {
          await navigator.serviceWorker.register($1);
          return 'registered';
        } catch (e) {
          return e.message;
        }
      })()
  )",
                                                         kScriptPath);
  EXPECT_EQ(base::StrCat(
                {"Failed to register a ServiceWorker: ", unsupported_origin}),
            content::EvalJs(web_contents(), register_script));

  // Nor can it reach a worker registered for it, to unregister one.
  constexpr char kGetRegistrationsScript[] = R"(
      (async () => {
        try {
          await navigator.serviceWorker.getRegistrations();
          return 'got registrations';
        } catch (e) {
          return e.message;
        }
      })()
  )";
  EXPECT_EQ(base::StrCat({"Failed to get ServiceWorkerRegistration objects: ",
                          unsupported_origin}),
            content::EvalJs(web_contents(), kGetRegistrationsScript));
}

// The point of the whole thing: an opted-in host's navigation is served by its
// service worker, and its document is controlled by it.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest,
                       ServiceWorkerServesNavigation) {
  const GURL scope = GetUntrustedURL(kInterceptedHost);
  AddWebUIConfig(scope, /*intercept_navigations=*/true);
  ASSERT_TRUE(RegisterAndActivateServiceWorker(scope));

  EXPECT_EQ(kServiceWorkerBody, NavigateAndGetBody(scope.Resolve("page")));
  EXPECT_EQ(true, IsControlled());
}

// A controlled document's subresources are reachable by the worker as well.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest,
                       ServiceWorkerServesSubresources) {
  const GURL scope = GetUntrustedURL(kInterceptedHost);
  AddWebUIConfig(scope, /*intercept_navigations=*/true);
  ASSERT_TRUE(RegisterAndActivateServiceWorker(scope));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), scope.Resolve("page")));

  EXPECT_EQ(kServiceWorkerBody,
            content::EvalJs(web_contents(),
                            "fetch('subresource').then(r => r.text())"));
  // A subresource the worker declines still comes from the data source.
  EXPECT_EQ(
      kDataSourceBody,
      content::EvalJs(web_contents(),
                      content::JsReplace("fetch($1).then(r => r.text())",
                                         base::StrCat({"/", kUnhandledPath}))));
}

// An opted-in host is no longer loaded by the shortcut, so the WebUI factory
// has to be reachable from the navigation's non-network factories: otherwise a
// request its worker declines would have nothing left to load it.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest,
                       NavigationFallsBackToWebUIFactory) {
  const GURL scope = GetUntrustedURL(kInterceptedHost);
  AddWebUIConfig(scope, /*intercept_navigations=*/true);
  ASSERT_TRUE(RegisterAndActivateServiceWorker(scope));

  EXPECT_EQ(kDataSourceBody, NavigateAndGetBody(scope.Resolve(kUnhandledPath)));
  // The document is still controlled, the worker just did not answer.
  EXPECT_EQ(true, IsControlled());
}

// Same, for an opted-in host with no worker registered at all.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest,
                       NavigationLoadsWithoutServiceWorker) {
  const GURL scope = GetUntrustedURL(kInterceptedHost);
  AddWebUIConfig(scope, /*intercept_navigations=*/true);

  EXPECT_EQ(kDataSourceBody, NavigateAndGetBody(scope.Resolve("page")));
  EXPECT_EQ(false, IsControlled());
}

// A host that has not opted in keeps the shortcut, so its navigation reaches no
// interceptor and its document gets no service worker client - even though a
// worker is registered for it.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest,
                       NavigationIsNotInterceptedWithoutOptIn) {
  const GURL scope = GetUntrustedURL(kNotInterceptedHost);
  AddWebUIConfig(scope, /*intercept_navigations=*/false);
  ASSERT_TRUE(RegisterAndActivateServiceWorker(scope));

  EXPECT_EQ(kDataSourceBody, NavigateAndGetBody(scope.Resolve("page")));
  EXPECT_EQ(false, IsControlled());
}

// Unregistering hands the host back to its data source.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest,
                       NavigationIsNotServedAfterUnregistering) {
  const GURL scope = GetUntrustedURL(kInterceptedHost);
  AddWebUIConfig(scope, /*intercept_navigations=*/true);
  ASSERT_TRUE(RegisterAndActivateServiceWorker(scope));
  ASSERT_EQ(kServiceWorkerBody, NavigateAndGetBody(scope.Resolve("page")));

  ASSERT_EQ(blink::ServiceWorkerStatusCode::kOk,
            UnregisterServiceWorker(scope));

  EXPECT_EQ(kDataSourceBody, NavigateAndGetBody(scope.Resolve("page")));
  EXPECT_EQ(false, IsControlled());
}

// Interception is opt-in, so every WebUI that has not asked for it is loaded
// exactly as before.
IN_PROC_BROWSER_TEST_F(WebUIServiceWorkerBrowserTest,
                       InterceptionIsOffByDefault) {
  ui::TestUntrustedWebUIConfig untrusted_config(kNotInterceptedHost);
  EXPECT_FALSE(untrusted_config.ShouldInterceptNavigationsWithServiceWorker());

  content::TestWebUIConfig trusted_config(kNotInterceptedHost);
  EXPECT_FALSE(trusted_config.ShouldInterceptNavigationsWithServiceWorker());
}
