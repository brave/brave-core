// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>

#include "base/functional/bind.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/search_engines/ui_thread_search_terms_data.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "url/gurl.h"

namespace {

constexpr char kSearchHost[] = "search.test";

// Where FontPrewarmerTabHelper caches the fonts of search results pages.
constexpr char kSearchResultsPageFontsPref[] =
    "cached_fonts.search_results_page.fonts";

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->set_content_type("text/html");
  response->set_content("<html><body style='font-family:Arial'>Results");
  return response;
}

}  // namespace

class BraveFontPrewarmerTabHelperBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    net::EmbeddedTestServer::ServerCertificateConfig cert_config;
    cert_config.dns_names = {kSearchHost};
    https_server_.SetSSLConfig(cert_config);
    https_server_.RegisterRequestHandler(base::BindRepeating(&HandleRequest));
    ASSERT_TRUE(https_server_.Start());

    // Make the test server the default search provider, so that its pages
    // count as search results pages.
    TemplateURLService* service =
        TemplateURLServiceFactory::GetForProfile(GetProfile());
    search_test_utils::WaitForTemplateURLServiceToLoad(service);
    TemplateURLData data;
    data.SetShortName(u"Test");
    data.SetKeyword(u"test");
    data.SetURL(
        https_server_.GetURL(kSearchHost, "/search?q={searchTerms}").spec());
    TemplateURL* search_provider =
        service->Add(std::make_unique<TemplateURL>(data));
    service->SetUserSelectedDefaultSearchProvider(search_provider);
  }

 protected:
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

// Brave does not create FontPrewarmerTabHelper (see
// rewrite/chrome/browser/ui/tabs/tab_features.cc.yaml), so visiting a search
// results page does not cache its fonts.
IN_PROC_BROWSER_TEST_F(BraveFontPrewarmerTabHelperBrowserTest,
                       SearchResultsPageFontsAreNotCached) {
  content::WebContents* web_contents =
      browser()->GetTabStripModel()->GetActiveWebContents();

  TemplateURLService* service =
      TemplateURLServiceFactory::GetForProfile(GetProfile());
  const GURL search_results_page_url =
      service->GetDefaultSearchProvider()->GenerateSearchURL(
          UIThreadSearchTermsData());
  ASSERT_TRUE(content::NavigateToURL(web_contents, search_results_page_url));
  // The helper would ask the renderer for the page's fonts as the navigation
  // commits. A further round trip to the same frame lets any reply arrive.
  ASSERT_TRUE(content::ExecJs(web_contents, "true"));

  EXPECT_TRUE(
      GetProfile()->GetPrefs()->GetList(kSearchResultsPageFontsPref).empty());
}
