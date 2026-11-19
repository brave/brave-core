// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_untrusted_conversation_ui.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/test/run_until.h"
#include "base/time/time.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/ai_chat_urls.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

constexpr char kAddIframeScript[] = R"(
  new Promise(resolve => {
    const iframe = document.createElement('iframe');
    iframe.onload = () => resolve(true);
    iframe.src = $1;
    document.body.appendChild(iframe);
  })
)";

constexpr char kLoadImageScript[] = R"(
  new Promise(resolve => {
    const img = new Image();
    img.onload = () => resolve(true);
    img.onerror = () => resolve(false);
    img.src = $1;
  })
)";

}  // namespace

// The conversation frame's images come from other chrome-untrusted:// origins,
// which only serve it if they allow its origin.
class AIChatUntrustedConversationUIBrowserTest : public PlatformBrowserTest {
 public:
  AIChatUntrustedConversationUIBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}
  ~AIChatUntrustedConversationUIBrowserTest() override = default;

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    https_server_.ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());

    content::WebContents* web_contents =
        chrome_test_utils::GetActiveWebContents(this);
    ASSERT_TRUE(content::NavigateToURL(web_contents, GURL(kAIChatUIURL)));

    const GURL frame_url =
        GURL(kAIChatUntrustedConversationUIURL).Resolve("image-test");
    ASSERT_EQ(true,
              content::EvalJs(web_contents,
                              content::JsReplace(kAddIframeScript, frame_url)));
    content::RenderFrameHost* conversation_frame =
        content::FrameMatchingPredicate(
            web_contents->GetPrimaryPage(),
            base::BindRepeating(&content::FrameHasSourceUrl, frame_url));
    ASSERT_TRUE(conversation_frame);
    conversation_frame_id_ = conversation_frame->GetGlobalId();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::EvalJsResult LoadImage(const std::string& src) {
    return content::EvalJs(
        content::RenderFrameHost::FromID(conversation_frame_id_),
        content::JsReplace(kLoadImageScript, src));
  }

 protected:
  net::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  // By id, since the frame is destroyed before the fixture at shutdown.
  content::GlobalRenderFrameHostId conversation_frame_id_;
};

IN_PROC_BROWSER_TEST_F(AIChatUntrustedConversationUIBrowserTest,
                       LoadsUntrustedFavicon) {
  EXPECT_EQ(true, LoadImage("chrome-untrusted://favicon2?size=64&pageUrl="
                            "https%3A%2F%2Fexample.com"));
}

IN_PROC_BROWSER_TEST_F(AIChatUntrustedConversationUIBrowserTest,
                       LoadsUntrustedSanitizedImage) {
  const GURL image_url = https_server_.GetURL("a.com", "/logo.png");
  EXPECT_EQ(true, LoadImage(base::StrCat(
                      {"chrome-untrusted://image?url=",
                       base::EscapeQueryParamValue(image_url.spec(),
                                                   /*use_plus=*/false)})));
}

// Web sources in a response are links from the conversation frame, so a site
// they lead to (B) must see a cross-site navigation, even via a redirect, and
// not a browser-initiated one, which would send its SameSite=Strict cookies and
// `Sec-Fetch-Site: none`. Runs on every platform, unlike the side panel
// variants in ai_chat_sidepanel_behavior_browsertest.cc.
class AIChatSourceLinkBrowserTest : public PlatformBrowserTest {
 public:
  AIChatSourceLinkBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}
  ~AIChatSourceLinkBrowserTest() override = default;

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    ai_chat::SetUserOptedIn(GetProfile()->GetPrefs(), true);

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    // For /echoheader and /server-redirect. Sec-Fetch-* headers are only sent
    // over HTTPS.
    net::test_server::RegisterDefaultHandlers(&https_server_);
    ASSERT_TRUE(https_server_.Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

 protected:
  struct EchoedHeaders {
    std::string cookie;
    std::string sec_fetch_site;
  };

  Profile* GetProfile() { return chrome_test_utils::GetProfile(this); }

  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  // Site B: echoes the `Cookie` and `Sec-Fetch-Site` headers, one per line.
  GURL GetSiteBEchoURL() {
    return https_server_.GetURL("b.com", "/echoheader?Cookie&Sec-Fetch-Site");
  }

  // Site C: redirects to `url`.
  GURL GetRedirectURL(const GURL& url) {
    return https_server_.GetURL(
        "c.com", base::StrCat({"/server-redirect?", url.spec()}));
  }

  void SetSameSiteCookies(const GURL& url) {
    ASSERT_TRUE(content::SetCookie(
        GetProfile(), url, "strict=1; SameSite=Strict; Secure; Path=/"));
    ASSERT_TRUE(content::SetCookie(GetProfile(), url,
                                   "lax=1; SameSite=Lax; Secure; Path=/"));
  }

  static EchoedHeaders GetEchoedHeaders(content::WebContents* contents) {
    const std::string body =
        content::EvalJs(contents, "document.body.innerText",
                        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                        ISOLATED_WORLD_ID_BRAVE_INTERNAL)
            .ExtractString();
    const std::vector<std::string> lines = base::SplitString(
        body, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    EXPECT_EQ(lines.size(), 2u) << body;
    if (lines.size() != 2u) {
      return {};
    }
    return {lines[0], lines[1]};
  }

  // Shows, in the active tab, a conversation whose response cites `source`,
  // and returns its conversation entries frame.
  content::RenderFrameHost* ShowConversationCiting(const GURL& source) {
    ai_chat::AIChatService* service =
        ai_chat::AIChatServiceFactory::GetForBrowserContext(GetProfile());
    EXPECT_TRUE(service);
    if (!service) {
      return nullptr;
    }
    ai_chat::ConversationHandler* conversation = service->CreateConversation();

    std::vector<ai_chat::mojom::ConversationTurnPtr> history;
    auto question = ai_chat::mojom::ConversationTurn::New();
    question->uuid = "question";
    question->character_type = ai_chat::mojom::CharacterType::HUMAN;
    question->action_type = ai_chat::mojom::ActionType::QUERY;
    question->text = "Question";
    question->created_time = base::Time::Now();
    history.push_back(std::move(question));

    // No citation in the text, so the source is only reachable via its link.
    auto answer = ai_chat::mojom::ConversationTurn::New();
    answer->uuid = "answer";
    answer->character_type = ai_chat::mojom::CharacterType::ASSISTANT;
    answer->action_type = ai_chat::mojom::ActionType::RESPONSE;
    answer->created_time = base::Time::Now();
    std::vector<ai_chat::mojom::WebSourcePtr> sources;
    sources.push_back(ai_chat::mojom::WebSource::New(
        "Source", source, GURL(), std::nullopt, std::nullopt));
    answer->events.emplace();
    answer->events->push_back(
        ai_chat::mojom::ConversationEntryEvent::NewCompletionEvent(
            ai_chat::mojom::CompletionEvent::New("An answer.")));
    answer->events->push_back(
        ai_chat::mojom::ConversationEntryEvent::NewSourcesEvent(
            ai_chat::mojom::WebSourcesEvent::New(std::move(sources),
                                                 std::vector<std::string>())));
    history.push_back(std::move(answer));
    conversation->SetChatHistoryForTesting(std::move(history));

    content::WebContents* contents = GetActiveWebContents();
    EXPECT_TRUE(content::NavigateToURL(
        contents,
        ai_chat::ConversationUrl(conversation->get_conversation_uuid())));

    content::RenderFrameHost* entries_frame = nullptr;
    EXPECT_TRUE(base::test::RunUntil([&]() {
      contents->ForEachRenderFrameHost([&](content::RenderFrameHost* frame) {
        if (frame->GetWebUI() && frame->GetWebUI()
                                     ->GetController()
                                     ->GetAs<AIChatUntrustedConversationUI>()) {
          entries_frame = frame;
        }
      });
      return entries_frame != nullptr;
    }));
    return entries_frame;
  }

  // Clicks the link to `url` labelled `text` once it renders, and returns the
  // new tab it opens once that reaches `final_url`.
  content::WebContents* FollowLinkToNewTab(content::RenderFrameHost* frame,
                                           const GURL& url,
                                           std::string_view text,
                                           const GURL& final_url) {
    constexpr char kWaitForLinkScript[] = R"(
      new Promise((resolve) => {
        const hasLink = () =>
            [...document.querySelectorAll('a[target=_blank]')].some(
                (a) => a.href === $1 && a.textContent.trim() === $2);
        if (hasLink()) {
          resolve(true);
          return;
        }
        const observer = new MutationObserver(() => {
          if (hasLink()) {
            observer.disconnect();
            resolve(true);
          }
        });
        observer.observe(document,
            {childList: true, subtree: true, characterData: true,
             attributes: true});
      })
    )";
    EXPECT_EQ(true,
              content::EvalJs(frame,
                              content::JsReplace(kWaitForLinkScript, url, text),
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL));

    constexpr char kClickLinkScript[] = R"(
      [...document.querySelectorAll('a[target=_blank]')]
          .find((a) => a.href === $1 && a.textContent.trim() === $2)
          .click()
    )";
    content::WebContentsAddedObserver new_tab_observer;
    content::TestNavigationObserver navigation_observer(final_url);
    navigation_observer.StartWatchingNewWebContents();
    EXPECT_TRUE(content::ExecJs(frame,
                                content::JsReplace(kClickLinkScript, url, text),
                                content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                ISOLATED_WORLD_ID_BRAVE_INTERNAL));
    navigation_observer.Wait();
    content::WebContents* new_tab = new_tab_observer.GetWebContents();
    EXPECT_EQ(new_tab->GetLastCommittedURL(), final_url);
    return new_tab;
  }

  net::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;
};

// Control: a typed navigation to B does send the Strict cookie.
IN_PROC_BROWSER_TEST_F(AIChatSourceLinkBrowserTest,
                       TypedNavigationSendsStrictCookie) {
  const GURL echo_url = GetSiteBEchoURL();
  SetSameSiteCookies(echo_url);

  ASSERT_TRUE(content::NavigateToURL(GetActiveWebContents(), echo_url));
  const EchoedHeaders headers = GetEchoedHeaders(GetActiveWebContents());
  EXPECT_EQ(headers.sec_fetch_site, "none") << "Cookie: " << headers.cookie;
  EXPECT_THAT(headers.cookie, testing::HasSubstr("strict=1"));
  EXPECT_THAT(headers.cookie, testing::HasSubstr("lax=1"));
}

IN_PROC_BROWSER_TEST_F(AIChatSourceLinkBrowserTest,
                       CrossSiteSourceLinkRedirectKeepsInitiator) {
  const GURL echo_url = GetSiteBEchoURL();
  SetSameSiteCookies(echo_url);

  const GURL source_url = GetRedirectURL(echo_url);
  content::RenderFrameHost* entries_frame = ShowConversationCiting(source_url);
  ASSERT_TRUE(entries_frame);

  content::WebContents* opened =
      FollowLinkToNewTab(entries_frame, source_url, "1 - c.com", echo_url);
  ASSERT_TRUE(opened);
  const EchoedHeaders headers = GetEchoedHeaders(opened);
  const std::string context =
      base::StrCat({"Cookie: ", headers.cookie,
                    ", Sec-Fetch-Site: ", headers.sec_fetch_site});
  EXPECT_EQ(headers.sec_fetch_site, "cross-site") << context;
  EXPECT_THAT(headers.cookie, testing::Not(testing::HasSubstr("strict=1")))
      << context;
  EXPECT_THAT(headers.cookie, testing::HasSubstr("lax=1")) << context;
}
