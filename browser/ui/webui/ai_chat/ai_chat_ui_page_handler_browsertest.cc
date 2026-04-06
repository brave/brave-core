// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/gtest_util.h"
#include "base/test/run_until.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ai_chat/tab_tracker_service_factory.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/dns/mock_host_resolver.h"
#include "pdf/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/url_constants.h"

namespace ai_chat {

#if !BUILDFLAG(IS_ANDROID)
constexpr char kExpectedTextContent[] =
    "Hello from a text file.\nThis is line two.";
#endif

#if BUILDFLAG(ENABLE_PDF)
constexpr char kExpectedPdfText[] = "This is the way\nI have spoken";
#endif  // BUILDFLAG(ENABLE_PDF)

class AIChatUIPageHandlerBrowserTest : public PlatformBrowserTest,
                                       public mojom::TabDataObserver {
 public:
  AIChatUIPageHandlerBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}
  ~AIChatUIPageHandlerBrowserTest() override = default;

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    // Setup TabTrackerService Observer
    auto* tracker =
        TabTrackerServiceFactory::GetInstance()->GetForBrowserContext(
            web_contents()->GetBrowserContext());
    mojo::PendingRemote<mojom::TabDataObserver> pending_remote;
    receiver_.Bind(pending_remote.InitWithNewPipeAndPassReceiver());
    tracker->AddObserver(std::move(pending_remote));

    https_server_.ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());

    ASSERT_TRUE(NavigateToURL(web_contents(), GURL("chrome://leo-ai")));
    EXPECT_TRUE(content::WaitForLoadStop(web_contents()));

    EXPECT_FALSE(web_contents()->GetController().NeedsReload());
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
    receiver_.reset();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void TabDataChanged(std::vector<mojom::TabDataPtr> tabs) override {
    tabs_ = std::move(tabs);
  }

  mojom::TabDataPtr GetTabDataForFirstMatchingURL(const GURL& url) {
    auto find_tab = [&]() {
      return std::find_if(tabs_.begin(), tabs_.end(),
                          [url](const auto& tab) { return tab->url == url; });
    };

    CHECK(base::test::RunUntil([&]() { return find_tab() != tabs_.end(); }));

    auto it = find_tab();
    if (it == tabs_.end()) {
      return nullptr;
    }
    return it->Clone();
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  AIChatUIPageHandler* GetPageHandler(content::WebContents* web_contents) {
    auto* webui =
        static_cast<AIChatUI*>(web_contents->GetWebUI()->GetController());
    EXPECT_TRUE(webui);
    // Wait for the page handler to be created. It's created when the frontend
    // binds to the mojom::AIChatUIHandler Mojo interface, which may not happen
    // immediately after the WebUI loads. Extend the timeout from the default 1s
    // to 30s for slower CI machines (especially macOS arm64 under load).
    base::test::ScopedRunLoopTimeout timeout(FROM_HERE, base::Seconds(30));
    bool handler_ready = base::test::RunUntil(
        [&]() { return webui->page_handler_.get() != nullptr; });
    EXPECT_TRUE(handler_ready);
    return webui->page_handler_.get();
  }

  void OpenNewTab() {
    GURL url = https_server_.GetURL("example.com", "/simple.html");
    EXPECT_TRUE(content::ExecJs(
        web_contents(),
        base::StrCat({"window.open('", url.spec(), "', '_blank');"})));
    content::WaitForLoadStop(web_contents());
    EXPECT_EQ(web_contents()->GetLastCommittedURL(), url);
  }

  std::pair<base::FilePath, std::vector<uint8_t>> ReadTestPdf() {
    base::FilePath pdf_path =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA)
            .AppendASCII("leo")
            .AppendASCII("dummy.pdf");
    std::optional<std::vector<uint8_t>> pdf_bytes;
    {
      base::ScopedAllowBlockingForTesting allow_blocking;
      pdf_bytes = base::ReadFileToBytes(pdf_path);
    }
    CHECK(pdf_bytes.has_value());
    return {pdf_path, std::move(*pdf_bytes)};
  }

 protected:
  std::vector<mojom::TabDataPtr> tabs_;

 private:
  mojo::Receiver<mojom::TabDataObserver> receiver_{this};

  net::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(AIChatUIPageHandlerBrowserTest,
                       WebContentsIsLoadedIfNeeded) {
  auto* ai_chat_contents = web_contents();
  ASSERT_TRUE(ai_chat_contents);
  EXPECT_EQ(ai_chat_contents->GetLastCommittedURL().scheme(),
            content::kChromeUIScheme);

  OpenNewTab();

  auto* contents_to_associate = web_contents();
  ASSERT_TRUE(contents_to_associate);

  // Set the NeedsReload flag - when the WebContents is associated we should
  // load it, clearing this flag.
  contents_to_associate->GetController().SetNeedsReload();

  auto* page_handler = GetPageHandler(ai_chat_contents);
  mojom::TabDataPtr tab_data = GetTabDataForFirstMatchingURL(
      contents_to_associate->GetLastCommittedURL());
  ASSERT_TRUE(tab_data);

  page_handler->AssociateTab(std::move(tab_data), "dont-know-dont-care");

  // Wait for the contents to finish loading and check the NeedsReload flag is
  // not set.
  EXPECT_TRUE(content::WaitForLoadStop(contents_to_associate));
  EXPECT_FALSE(contents_to_associate->GetController().NeedsReload());
}

IN_PROC_BROWSER_TEST_F(AIChatUIPageHandlerBrowserTest,
                       WebContentsDestroyedWhileAssociatingDoesNotCrash) {
  auto* ai_chat_contents = web_contents();
  ASSERT_TRUE(ai_chat_contents);
  EXPECT_EQ(ai_chat_contents->GetLastCommittedURL().scheme(),
            content::kChromeUIScheme);

  OpenNewTab();

  auto* contents_to_associate = web_contents();
  ASSERT_TRUE(contents_to_associate);

  // Set the NeedsReload flag so the Association doesn't happen immediately
  contents_to_associate->GetController().SetNeedsReload();

  auto* page_handler = GetPageHandler(ai_chat_contents);
  mojom::TabDataPtr tab_data = GetTabDataForFirstMatchingURL(
      contents_to_associate->GetLastCommittedURL());
  ASSERT_TRUE(tab_data);

  page_handler->AssociateTab(std::move(tab_data), "dont-know-dont-care");

  // Close the web contents while the association is in progress
  contents_to_associate->Close();
}

IN_PROC_BROWSER_TEST_F(AIChatUIPageHandlerBrowserTest,
                       WebUIClosedWhileAssociatingDoesNotCrash) {
  auto* ai_chat_contents = web_contents();
  ASSERT_TRUE(ai_chat_contents);
  EXPECT_EQ(ai_chat_contents->GetLastCommittedURL().scheme(),
            content::kChromeUIScheme);

  OpenNewTab();

  auto* contents_to_associate = web_contents();
  ASSERT_TRUE(contents_to_associate);

  // Set the NeedsReload flag so the Association doesn't happen immediately
  contents_to_associate->GetController().SetNeedsReload();

  auto* page_handler = GetPageHandler(ai_chat_contents);
  mojom::TabDataPtr tab_data = GetTabDataForFirstMatchingURL(
      contents_to_associate->GetLastCommittedURL());
  ASSERT_TRUE(tab_data);

  page_handler->AssociateTab(std::move(tab_data), "dont-know-dont-care");

  // Close the WebUI while the association is in progress
  ai_chat_contents->Close();
}

IN_PROC_BROWSER_TEST_F(AIChatUIPageHandlerBrowserTest, ProcessImageFile) {
  auto* ai_chat_contents = web_contents();
  ASSERT_TRUE(ai_chat_contents);

  auto* page_handler = GetPageHandler(ai_chat_contents);
  ASSERT_TRUE(page_handler);

  // Test with invalid image data - should result in null
  std::vector<uint8_t> invalid_data = {1, 2, 3, 4};
  base::test::TestFuture<ai_chat::mojom::UploadedFilePtr> future_invalid;

  page_handler->ProcessImageFile(invalid_data, "test.png",
                                 future_invalid.GetCallback());

  auto invalid_result = future_invalid.Take();
  EXPECT_FALSE(invalid_result);  // Should be null for invalid data

  // Test with valid PNG image data - should succeed
  constexpr uint8_t kValidPng[] = {
      0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00,
      0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
      0x00, 0x01, 0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53, 0xde,
      0x00, 0x00, 0x00, 0x10, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x62,
      0x5a, 0xc4, 0x5e, 0x08, 0x08, 0x00, 0x00, 0xff, 0xff, 0x02, 0x71,
      0x01, 0x1d, 0xcd, 0xd0, 0xd6, 0x62, 0x00, 0x00, 0x00, 0x00, 0x49,
      0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

  std::vector<uint8_t> valid_data(std::begin(kValidPng), std::end(kValidPng));
  base::test::TestFuture<ai_chat::mojom::UploadedFilePtr> future_valid;

  page_handler->ProcessImageFile(valid_data, "valid.png",
                                 future_valid.GetCallback());

  auto valid_result = future_valid.Take();
  ASSERT_TRUE(valid_result);  // Should succeed with valid PNG data
  EXPECT_EQ(valid_result->filename, "valid.png");
  EXPECT_EQ(valid_result->type, ai_chat::mojom::UploadedFileType::kImage);
  EXPECT_GT(valid_result->data.size(), 0u);
  EXPECT_EQ(valid_result->filesize, valid_result->data.size());
}

#if BUILDFLAG(ENABLE_PDF)
IN_PROC_BROWSER_TEST_F(AIChatUIPageHandlerBrowserTest, ProcessPdfFile) {
  auto* page_handler = GetPageHandler(web_contents());
  ASSERT_TRUE(page_handler);

  auto [pdf_path, pdf_bytes] = ReadTestPdf();

  base::test::TestFuture<ai_chat::mojom::UploadedFilePtr> future;
  page_handler->ProcessPdfFile(pdf_bytes, "dummy.pdf", future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result);
  EXPECT_EQ(result->filename, "dummy.pdf");
  EXPECT_EQ(result->type, ai_chat::mojom::UploadedFileType::kPdf);
  EXPECT_GT(result->data.size(), 0u);
  ASSERT_TRUE(result->extracted_text.has_value());
  EXPECT_EQ(*result->extracted_text, kExpectedPdfText);
}

IN_PROC_BROWSER_TEST_F(AIChatUIPageHandlerBrowserTest,
                       OnFilesUploaded_WithPdf) {
  auto* page_handler = GetPageHandler(web_contents());
  ASSERT_TRUE(page_handler);

  auto [pdf_path, pdf_bytes] = ReadTestPdf();

  // Build UploadedFile with full path as filename (simulating what
  // UploadFileHelper returns from the file picker).
  std::vector<ai_chat::mojom::UploadedFilePtr> files;
  files.push_back(ai_chat::mojom::UploadedFile::New(
      pdf_path.AsUTF8Unsafe(), pdf_bytes.size(), std::move(pdf_bytes),
      ai_chat::mojom::UploadedFileType::kPdf, std::nullopt));

  base::test::TestFuture<
      std::optional<std::vector<ai_chat::mojom::UploadedFilePtr>>>
      future;
  page_handler->OnFilesUploaded(future.GetCallback(),
                                std::make_optional(std::move(files)));

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1u);
  EXPECT_EQ((*result)[0]->filename, "dummy.pdf");
  ASSERT_TRUE((*result)[0]->extracted_text.has_value());
  EXPECT_EQ(*(*result)[0]->extracted_text, kExpectedPdfText);
}
#endif  // BUILDFLAG(ENABLE_PDF)

#if !BUILDFLAG(IS_ANDROID)
IN_PROC_BROWSER_TEST_F(AIChatUIPageHandlerBrowserTest, ProcessTextFile) {
  auto* page_handler = GetPageHandler(web_contents());
  ASSERT_TRUE(page_handler);

  base::FilePath txt_path;
  std::vector<uint8_t> txt_bytes;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    txt_path = base::PathService::CheckedGet(brave::DIR_TEST_DATA)
                   .AppendASCII("leo")
                   .AppendASCII("dummy.txt");
    auto bytes = base::ReadFileToBytes(txt_path);
    ASSERT_TRUE(bytes.has_value());
    txt_bytes = std::move(*bytes);
  }

  base::test::TestFuture<ai_chat::mojom::UploadedFilePtr> future;
  page_handler->ProcessTextFile(txt_bytes, "dummy.txt", future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result);
  EXPECT_EQ(result->filename, "dummy.txt");
  EXPECT_EQ(result->type, ai_chat::mojom::UploadedFileType::kText);
  EXPECT_GT(result->data.size(), 0u);
  ASSERT_TRUE(result->extracted_text.has_value());
  EXPECT_EQ(*result->extracted_text, kExpectedTextContent);
}

// Verify that HTML files are not rendered (no script execution, no external
// resource loading). The extracted text must be the raw HTML source.
IN_PROC_BROWSER_TEST_F(AIChatUIPageHandlerBrowserTest, ProcessHtmlFile) {
  auto* page_handler = GetPageHandler(web_contents());
  ASSERT_TRUE(page_handler);

  base::FilePath html_path;
  std::vector<uint8_t> html_bytes;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    html_path = base::PathService::CheckedGet(brave::DIR_TEST_DATA)
                    .AppendASCII("leo")
                    .AppendASCII("dummy.html");
    auto bytes = base::ReadFileToBytes(html_path);
    ASSERT_TRUE(bytes.has_value());
    html_bytes = std::move(*bytes);
  }

  base::test::TestFuture<ai_chat::mojom::UploadedFilePtr> future;
  page_handler->ProcessTextFile(html_bytes, "dummy.html", future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result);
  EXPECT_EQ(result->filename, "dummy.html");
  ASSERT_TRUE(result->extracted_text.has_value());
  // Raw source must contain HTML tags — proves it was NOT rendered.
  EXPECT_TRUE(result->extracted_text->find("<p>Hello from an HTML file.</p>") !=
              std::string::npos);
  EXPECT_TRUE(result->extracted_text->find("<script>") != std::string::npos);
}

IN_PROC_BROWSER_TEST_F(AIChatUIPageHandlerBrowserTest,
                       OnFilesUploaded_WithText) {
  auto* page_handler = GetPageHandler(web_contents());
  ASSERT_TRUE(page_handler);

  base::FilePath txt_path;
  std::vector<uint8_t> txt_bytes;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    txt_path = base::PathService::CheckedGet(brave::DIR_TEST_DATA)
                   .AppendASCII("leo")
                   .AppendASCII("dummy.txt");
    auto bytes = base::ReadFileToBytes(txt_path);
    ASSERT_TRUE(bytes.has_value());
    txt_bytes = std::move(*bytes);
  }

  std::vector<ai_chat::mojom::UploadedFilePtr> files;
  files.push_back(ai_chat::mojom::UploadedFile::New(
      txt_path.AsUTF8Unsafe(), txt_bytes.size(), std::move(txt_bytes),
      ai_chat::mojom::UploadedFileType::kText, std::nullopt));

  base::test::TestFuture<
      std::optional<std::vector<ai_chat::mojom::UploadedFilePtr>>>
      future;
  page_handler->OnFilesUploaded(future.GetCallback(),
                                std::make_optional(std::move(files)));

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1u);
  EXPECT_EQ((*result)[0]->filename, "dummy.txt");
  ASSERT_TRUE((*result)[0]->extracted_text.has_value());
  EXPECT_EQ(*(*result)[0]->extracted_text, kExpectedTextContent);
}
#endif  // !BUILDFLAG(IS_ANDROID)

IN_PROC_BROWSER_TEST_F(AIChatUIPageHandlerBrowserTest,
                       AssociateURLDoesNotCrashShutdown) {
  auto* ai_chat_contents = web_contents();
  ASSERT_TRUE(ai_chat_contents);
  EXPECT_EQ(ai_chat_contents->GetLastCommittedURL().scheme(),
            content::kChromeUIScheme);

  OpenNewTab();

  auto* contents_to_associate = web_contents();
  ASSERT_TRUE(contents_to_associate);

  auto* page_handler = GetPageHandler(ai_chat_contents);
  auto* service = AIChatServiceFactory::GetForBrowserContext(GetProfile());
  auto* conversation = service->CreateConversation();

  page_handler->AssociateUrlContent(GURL("https://example.com"), "Example",
                                    conversation->get_conversation_uuid());
  EXPECT_EQ(
      conversation->associated_content_manager()->GetAssociatedContent().size(),
      1u);

  // Note: We could crash while the profile is destroyed because
  // `AssociatedUrlContent` (which is owned by `AssociatedContentManager` <==
  // `ConversationHandler` <== `AIChatService`) holds a
  // std::unique_ptr<content::WebContents>, which needs to be destroyed before
  // the profile destructor is called. This test is essentially checking that
  // its destroyed during the profile shutdown phase, rather than destruction.
}

}  // namespace ai_chat
