/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/browser/extensions/brave_extension_functional_test.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page_ui.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/search_engines/ntp_search_engine_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_test_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/child_process_termination_info.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_process_host_observer.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/result_codes.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace {

class RenderProcessExitObserver : public content::RenderProcessHostObserver {
 public:
  explicit RenderProcessExitObserver(content::RenderProcessHost* host)
      : render_process_host_(host) {
    CHECK(render_process_host_);
    render_process_host_->AddObserver(this);
  }

  ~RenderProcessExitObserver() override { Reset(); }

 protected:
  // content::RenderProcessHostObserver:
  void RenderProcessExited(
      content::RenderProcessHost* host,
      const content::ChildProcessTerminationInfo& info) override {
    // Ensure the process exited normally and not due to a crash.
    EXPECT_EQ(info.exit_code, content::RESULT_CODE_NORMAL_EXIT);
  }

  void RenderProcessHostDestroyed(content::RenderProcessHost* host) override {
    Reset();
  }

  void Reset() {
    if (render_process_host_) {
      render_process_host_->RemoveObserver(this);
      render_process_host_ = nullptr;
    }
  }

  raw_ptr<content::RenderProcessHost> render_process_host_ = nullptr;
};

void VerifyDocumentBodyInnerTextExpectation(
    content::WebContents* web_contents,
    const std::string& expected_inner_text) {
  EXPECT_EQ(expected_inner_text,
            content::EvalJs(web_contents, "document.body.innerText;")
                .ExtractString());
}

void VerifyNewTabPageLoadedExpectation(content::WebContents* web_contents) {
  EXPECT_TRUE(content::EvalJs(web_contents,
                              "!!document.querySelector(`html[data-testid="
                              "'brave-new-tab-page']`)")
                  .ExtractBool());
}

void SimulateGoBack(content::WebContents* web_contents) {
  content::TestNavigationObserver observer(
      web_contents, /*expected_number_of_navigations=*/1);
  web_contents->GetController().GoBack();
  observer.Wait();
  EXPECT_TRUE(WaitForLoadStop(web_contents));
}

}  // namespace

class BraveNewTabPageUIBrowserTest
    : public extensions::ExtensionFunctionalTest {
 protected:
  content::WebContents* GetActiveWebContents() {
    content::WebContents* web_contents =
        chrome_test_utils::GetActiveWebContents(this);
    EXPECT_TRUE(web_contents);
    return web_contents;
  }

  void SimulateNavigateToUrlAndWaitForLoad(content::WebContents* web_contents,
                                           const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    EXPECT_TRUE(WaitForLoadStop(web_contents));
  }

  void SimulateOpenNewTabAndWaitForLoad(content::WebContents* web_contents) {
    SimulateNavigateToUrlAndWaitForLoad(web_contents,
                                        GURL(chrome::kChromeUINewTabURL));
  }
};

IN_PROC_BROWSER_TEST_F(BraveNewTabPageUIBrowserTest, CustomSearchEngines) {
  namespace mojom = brave_new_tab_page_refresh::mojom;
  auto* service =
      TemplateURLServiceFactory::GetForProfile(browser()->GetProfile());
  service->Load();
  TemplateURLServiceLoadWaiter().WaitForLoadComplete(*service);

  for (const auto& [id, engine] : TemplateURLPrepopulateData::kBraveEngines) {
    TemplateURLData data;
    data.SetShortName(engine->name);
    data.SetKeyword(engine->keyword);
    data.SetURL(engine->search_url);
    data.prepopulate_id = id;
    data.search_url_post_params =
        engine->search_url_post_params ? engine->search_url_post_params : "";
    EXPECT_TRUE(
        ntp_search::IsEligibleNtpSearchEngine(TemplateURL(data), *service))
        << id;
  }

  TemplateURLData extension_data;
  extension_data.SetShortName(u"Extension");
  extension_data.SetKeyword(u"extension-search");
  extension_data.SetURL("https://example.com/?q={searchTerms}");
  extension_data.is_active = TemplateURLData::ActiveStatus::kTrue;
  TemplateURL extension_engine(extension_data,
                               TemplateURL::NORMAL_CONTROLLED_BY_EXTENSION,
                               "test-extension", base::Time(), false);
  EXPECT_FALSE(
      ntp_search::IsEligibleNtpSearchEngine(extension_engine, *service));
  extension_data.starter_pack_id = 1;
  EXPECT_FALSE(ntp_search::IsEligibleNtpSearchEngine(
      TemplateURL(extension_data), *service));

  auto add_engine = [&](const std::u16string& keyword, const std::string& path,
                        TemplateURLData::ActiveStatus active,
                        const std::string& post_params = "") {
    TemplateURLData data;
    data.SetShortName(keyword);
    data.SetKeyword(keyword);
    data.SetURL(embedded_test_server()->GetURL(path).spec());
    data.is_active = active;
    data.search_url_post_params = post_params;
    service->Add(std::make_unique<TemplateURL>(data));
  };
  add_engine(u"custom-web", "/search?q={searchTerms}",
             TemplateURLData::ActiveStatus::kTrue);
  add_engine(u"custom-images", "/images?q={searchTerms}",
             TemplateURLData::ActiveStatus::kTrue);
  add_engine(u"inactive-search", "/inactive?q={searchTerms}",
             TemplateURLData::ActiveStatus::kFalse);
  add_engine(u"discovered-search", "/discovered?q={searchTerms}",
             TemplateURLData::ActiveStatus::kUnspecified);
  add_engine(u"static-shortcut", "/simple.html",
             TemplateURLData::ActiveStatus::kTrue);
  add_engine(u"post-search", "/post",
             TemplateURLData::ActiveStatus::kTrue, "q={searchTerms}");

  auto* contents = GetActiveWebContents();
  SimulateOpenNewTabAndWaitForLoad(contents);
  auto* controller =
      contents->GetWebUI()->GetController()->GetAs<BraveNewTabPageUI>();
  ASSERT_TRUE(controller);
  mojo::Remote<mojom::NewTabPageHandler> handler;
  controller->BindInterface(handler.BindNewPipeAndPassReceiver());

  base::test::TestFuture<std::vector<mojom::SearchEngineInfoPtr>> engines;
  handler->GetAvailableSearchEngines(engines.GetCallback());
  bool found_web = false;
  bool found_images = false;
  for (const auto& engine : engines.Get()) {
    found_web |= engine->keyword == "custom-web";
    found_images |= engine->keyword == "custom-images";
    EXPECT_NE(engine->keyword, "inactive-search");
    EXPECT_NE(engine->keyword, "discovered-search");
    EXPECT_NE(engine->keyword, "static-shortcut");
    EXPECT_NE(engine->keyword, "post-search");
  }
  EXPECT_TRUE(found_web);
  EXPECT_TRUE(found_images);

  auto* web_engine = service->GetTemplateURLForKeyword(u"custom-web");
  ASSERT_TRUE(web_engine);
  service->SetIsActiveTemplateURL(web_engine, false);

  const GURL initial_url = contents->GetLastCommittedURL();
  for (const auto* id : {"keyword:custom-web", "keyword:inactive-search",
                         "keyword:discovered-search", "keyword:static-shortcut",
                         "keyword:post-search", "keyword:deleted-engine"}) {
    base::test::TestFuture<void> rejected;
    handler->OpenSearch("trees", id, mojom::EventDetails::New(),
                        rejected.GetCallback());
    ASSERT_TRUE(rejected.Wait());
    EXPECT_EQ(contents->GetLastCommittedURL(), initial_url);
    EXPECT_EQ(ntp_search::FindNtpSearchEngine(id, *service), nullptr);
  }

  content::TestNavigationObserver navigation(contents);
  base::test::TestFuture<void> opened;
  handler->OpenSearch("hello world", "keyword:custom-images",
                      mojom::EventDetails::New(), opened.GetCallback());
  ASSERT_TRUE(opened.Wait());
  navigation.Wait();
  EXPECT_EQ(contents->GetLastCommittedURL(),
            embedded_test_server()->GetURL("/images?q=hello+world"));
}

// Test that properties are set on the correct RenderViewHost.
IN_PROC_BROWSER_TEST_F(BraveNewTabPageUIBrowserTest, StartupURLTest) {
  content::WebContents* web_contents = GetActiveWebContents();

  content::RenderProcessHost* render_process_host =
      web_contents->GetPrimaryMainFrame()->GetProcess();
  RenderProcessExitObserver observer(render_process_host);

  SimulateOpenNewTabAndWaitForLoad(web_contents);
  VerifyNewTabPageLoadedExpectation(web_contents);

  SimulateNavigateToUrlAndWaitForLoad(
      web_contents, embedded_test_server()->GetURL("/simple.html"));
  VerifyDocumentBodyInnerTextExpectation(web_contents, "Non empty simple page");

  SimulateGoBack(web_contents);
  VerifyNewTabPageLoadedExpectation(web_contents);
}

// This test simply checks that by default the Brave new tab page is used.
// It does this by loading the newtab page and then checking if
// window.brave_new_tab exists.
IN_PROC_BROWSER_TEST_F(BraveNewTabPageUIBrowserTest, BraveNewTabIsDefault) {
  content::WebContents* web_contents = GetActiveWebContents();
  SimulateOpenNewTabAndWaitForLoad(web_contents);
  VerifyNewTabPageLoadedExpectation(web_contents);
}

// This test simply loads an extension that sets a newtab override.
// It checks to make sure the newtab override is used as the newtab page.
IN_PROC_BROWSER_TEST_F(BraveNewTabPageUIBrowserTest,
                       NewTabPageLocationOverride) {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  InstallExtensionSilently(extension_service(),
                           test_data_dir.AppendASCII("new_tab_override.crx"));

  content::WebContents* web_contents = GetActiveWebContents();
  SimulateOpenNewTabAndWaitForLoad(web_contents);
  VerifyDocumentBodyInnerTextExpectation(web_contents, "New tab override!");
}

IN_PROC_BROWSER_TEST_F(BraveNewTabPageUIBrowserTest,
                       NonPrimaryOTRProfileShowsBlankPage) {
  Profile* otr_profile = browser()->GetProfile()->GetOffTheRecordProfile(
      Profile::OTRProfileID::CreateUniqueForDevTools(),
      /*create_if_needed=*/true);
  auto* otr_browser = CreateBrowser(otr_profile);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(otr_browser,
                                           GURL(chrome::kChromeUINewTabURL)));
  content::WebContents* web_contents =
      otr_browser->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(web_contents));
  EXPECT_FALSE(content::EvalJs(web_contents,
                               "!!document.querySelector(`html[data-testid="
                               "'brave-new-tab-page']`)")
                   .ExtractBool());
  VerifyDocumentBodyInnerTextExpectation(web_contents, "");
  CloseBrowserSynchronously(otr_browser);
}
