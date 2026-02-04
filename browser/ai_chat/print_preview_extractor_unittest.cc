// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/print_preview_extractor.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/rand_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/types/expected.h"
#include "brave/browser/ai_chat/print_preview_extractor_internal.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "brave/services/printing/public/mojom/pdf_to_bitmap_converter.mojom.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/prefs/pref_service.h"
#include "components/printing/browser/print_composite_client.h"
#include "components/printing/common/print.mojom.h"
#include "content/public/test/web_contents_tester.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"
#include "printing/buildflags/buildflags.h"
#include "printing/print_job_constants.h"
#include "printing/units.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "ui/gfx/image/image_unittest_util.h"

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

namespace ai_chat {

namespace {

base::MappedReadOnlyRegion CreatePageRegion(size_t size) {
  // We need to fulfill printing::LooksLikePdf()
  base::MappedReadOnlyRegion page =
      base::ReadOnlySharedMemoryRegion::Create(size < 50u ? 50u : size);
  auto writer = base::SpanWriter(base::span(page.mapping));
  writer.Write(base::byte_span_from_cstring("%PDF-"));
  base::RandBytes(writer.remaining_span());
  return page;
}

class MockPrintPreviewPrintRenderFrame
    : public printing::mojom::PrintRenderFrame {
 public:
  explicit MockPrintPreviewPrintRenderFrame(
      blink::AssociatedInterfaceProvider* provider) {
    provider->OverrideBinderForTesting(
        printing::mojom::PrintRenderFrame::Name_,
        base::BindRepeating(
            &MockPrintPreviewPrintRenderFrame::BindPrintRenderFrameReceiver,
            base::Unretained(this)));
  }
  MockPrintPreviewPrintRenderFrame(const MockPrintPreviewPrintRenderFrame&) =
      delete;
  MockPrintPreviewPrintRenderFrame& operator=(
      const MockPrintPreviewPrintRenderFrame&) = delete;
  MockPrintPreviewPrintRenderFrame(MockPrintPreviewPrintRenderFrame&&) = delete;
  MockPrintPreviewPrintRenderFrame& operator=(
      MockPrintPreviewPrintRenderFrame&&) = delete;
  ~MockPrintPreviewPrintRenderFrame() override = default;

  enum class ExpectedError {
    kNone,
    kPrintPreviewFailed,
    kPrintPreviewCanceled,
    kPrinterSettingsInvalid
  };
  void SetExpectedError(ExpectedError error) { expected_error_ = error; }

  // printing::mojom::PrintRenderFrame:
  void PrintRequestedPages() override {}
  void PrintWithParams(printing::mojom::PrintPagesParamsPtr params,
                       PrintWithParamsCallback callback) override {}
  void PrintForSystemDialog() override {}
  void SetPrintPreviewUI(
      mojo::PendingAssociatedRemote<printing::mojom::PrintPreviewUI> preview)
      override {
    preview_ui_.Bind(std::move(preview));
  }
  void InitiatePrintPreview(
#if BUILDFLAG(IS_CHROMEOS)
      mojo::PendingAssociatedRemote<mojom::PrintRenderer> print_renderer,
#endif
      bool has_selection) override {
  }
  void PrintPreview(base::DictValue settings) override {
    settings_ = std::move(settings);
    if (closure_) {
      std::move(closure_).Run();
    }
    switch (expected_error_) {
      case ExpectedError::kNone:
        break;
      case ExpectedError::kPrintPreviewFailed:
        preview_ui_->PrintPreviewFailed(0, 0);
        break;
      case ExpectedError::kPrintPreviewCanceled:
        preview_ui_->PrintPreviewCancelled(0, 0);
        break;
      case ExpectedError::kPrinterSettingsInvalid:
        preview_ui_->PrinterSettingsInvalid(0, 0);
        break;
    }
  }

  void OnPrintPreviewDialogClosed() override {}
  void PrintFrameContent(printing::mojom::PrintFrameContentParamsPtr params,
                         PrintFrameContentCallback callback) override {}
  void PrintingDone(bool success) override {}
  void ConnectToPdfRenderer() override {}
  void PrintNodeUnderContextMenu() override {}
  void SetIsPrintPreviewExtraction(bool value) override {}

  void BindPrintRenderFrameReceiver(
      mojo::ScopedInterfaceEndpointHandle handle) {
    receiver_.Bind(
        mojo::PendingAssociatedReceiver<printing::mojom::PrintRenderFrame>(
            std::move(handle)));
  }

  const base::DictValue& GetSettings() { return settings_; }

  void SetPrintPreviewCalledClosure(base::OnceClosure closure) {
    closure_ = std::move(closure);
  }

 private:
  base::OnceClosure closure_;
  base::DictValue settings_;
  ExpectedError expected_error_ = ExpectedError::kNone;

  mojo::AssociatedRemote<printing::mojom::PrintPreviewUI> preview_ui_;
  mojo::AssociatedReceiver<printing::mojom::PrintRenderFrame> receiver_{this};
};

class MockPreviewPageImageExtractor : public PreviewPageImageExtractor {
 public:
  MockPreviewPageImageExtractor(
      base::ReadOnlySharedMemoryRegion expected_region,
      bool expected_error)
      : expected_region_(std::move(expected_region)),
        expected_error_(expected_error) {}
  ~MockPreviewPageImageExtractor() override = default;

  MockPreviewPageImageExtractor(const MockPreviewPageImageExtractor&) = delete;
  MockPreviewPageImageExtractor& operator=(
      const MockPreviewPageImageExtractor&) = delete;

  void StartExtract(
      base::ReadOnlySharedMemoryRegion pdf_region,
      PrintPreviewExtractor::Extractor::ImageCallback callback,
      std::optional<bool> pdf_use_skia_renderer_enabled) override {
    // verify the correct memory region is passed
    EXPECT_EQ(pdf_region.Map().GetMemoryAsSpan<const uint8_t>(),
              expected_region_.Map().GetMemoryAsSpan<const uint8_t>());

    if (!expected_error_) {
      std::vector<std::vector<uint8_t>> result = {{0xde, 0xad}, {0xbe, 0xef}};
      std::move(callback).Run(base::ok(std::move(result)));
    } else {
      std::move(callback).Run(
          base::unexpected("PreviewPageImageExtractor error"));
    }
  }

 private:
  base::ReadOnlySharedMemoryRegion expected_region_;
  bool expected_error_ = false;
};

class MockPdfToBitmapConverter : public printing::mojom::PdfToBitmapConverter {
 public:
  MockPdfToBitmapConverter() = default;
  ~MockPdfToBitmapConverter() override = default;

  MockPdfToBitmapConverter(const MockPdfToBitmapConverter&) = delete;
  MockPdfToBitmapConverter& operator=(const MockPdfToBitmapConverter&) = delete;

  // printing::mojom::PdfToBitmapConverter:
  void GetPdfPageCount(base::ReadOnlySharedMemoryRegion pdf_region,
                       GetPdfPageCountCallback callback) override {
    std::move(callback).Run(expected_page_count_);
  }

  void GetBitmap(base::ReadOnlySharedMemoryRegion pdf_region,
                 uint32_t page_index,
                 GetBitmapCallback callback) override {
    if (expected_empty_bitmap_) {
      std::move(callback).Run(SkBitmap());
    } else {
      // Set dimension <= 2x2 to fail OCR intentionally
      std::move(callback).Run(gfx::test::CreateBitmap(2));
    }
  }

  void SetUseSkiaRendererPolicy(bool use_skia) override {}

  mojo::PendingRemote<printing::mojom::PdfToBitmapConverter> Bind() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void SetExpectedPageCount(std::optional<uint32_t> count) {
    expected_page_count_ = count;
  }

  void SetExpectedEmptyBitmap(bool empty_bitmap) {
    expected_empty_bitmap_ = empty_bitmap;
  }

 private:
  std::optional<uint32_t> expected_page_count_;
  bool expected_empty_bitmap_ = true;
  mojo::Receiver<printing::mojom::PdfToBitmapConverter> receiver_{this};
};

}  // namespace

using ImageResult =
    base::expected<std::vector<std::vector<uint8_t>>, std::string>;

class PrintPreviewExtractorTest : public ChromeRenderViewHostTestHarness {
 public:
  PrintPreviewExtractorTest() = default;
  ~PrintPreviewExtractorTest() override = default;

  PrintPreviewExtractorTest(const PrintPreviewExtractorTest&) = delete;
  PrintPreviewExtractorTest& operator=(const PrintPreviewExtractorTest&) =
      delete;

 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    NavigateAndCommit(GURL("https://brave.com/"),
                      ui::PageTransition::PAGE_TRANSITION_FIRST);
    printing::PrintCompositeClient::CreateForWebContents(web_contents());
    pp_extractor_ = std::make_unique<PrintPreviewExtractor>(
        web_contents(),
        base::BindRepeating([](content::WebContents* web_contents, bool is_pdf,
                               ai_chat::PrintPreviewExtractor::Extractor::
                                   ImageCallback&& callback)
                                -> std::unique_ptr<
                                    ai_chat::PrintPreviewExtractor::Extractor> {
          return std::make_unique<ai_chat::PrintPreviewExtractorInternal>(
              web_contents,
              Profile::FromBrowserContext(web_contents->GetBrowserContext()),
              is_pdf, std::move(callback),
              base::BindRepeating(
                  []() -> base::IDMap<printing::mojom::PrintPreviewUI*>& {
                    return printing::PrintPreviewUI::GetPrintPreviewUIIdMap();
                  }),
              base::BindRepeating([]() -> base::flat_map<int, int>& {
                return printing::PrintPreviewUI::
                    GetPrintPreviewUIRequestIdMap();
              }));
        }));
  }
  void TearDown() override {
    pp_extractor_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  blink::AssociatedInterfaceProvider*
  GetInitiatorAssociatedInterfaceProvider() {
    return web_contents()
        ->GetPrimaryMainFrame()
        ->GetRemoteAssociatedInterfaces();
  }

  std::optional<int32_t> GetPrintPreviewUIId() {
    return pp_extractor_->extractor_
               ? pp_extractor_->extractor_->GetPrintPreviewUIIdForTesting()
               : std::nullopt;
  }

  // Helper method for common setup and verification
  std::unique_ptr<MockPrintPreviewPrintRenderFrame> SetupPrintPreviewTest(
      const std::string& mime_type,
      MockPrintPreviewPrintRenderFrame::ExpectedError expected_error,
      base::OnceClosure on_complete = base::OnceClosure()) {
    content::WebContentsTester::For(web_contents())
        ->SetMainFrameMimeType(mime_type);

    auto print_render_frame =
        std::make_unique<MockPrintPreviewPrintRenderFrame>(
            GetInitiatorAssociatedInterfaceProvider());
    print_render_frame->SetExpectedError(expected_error);
    if (!on_complete.is_null()) {
      print_render_frame->SetPrintPreviewCalledClosure(std::move(on_complete));
    }
    return print_render_frame;
  }

  // Intentionally setup a PrintRenderFrame error to stop early so we can
  // verify the print settings we pass is correct.
  void RunPrintSettingsTest(const std::string& mime_type,
                            bool expect_preview_modifiable,
                            const base::Location& location = FROM_HERE) {
    SCOPED_TRACE(location.ToString());

    constexpr char kSettingTemplate[] =
        R"({
    "headerFooterEnabled": false,
    "shouldPrintBackgrounds": false,
    "shouldPrintSelectionOnly": false,
    "marginsType": %d,
    "collate": true,
    "copies": 1,
    "color": %d,
    "dpiHorizontal": %d,
    "dpiVertical": %d,
    "duplex": %d,
    "landscape": false,
    "deviceName": "",
    "printerType": %d,
    "scaleFactor": 100,
    "rasterizePDF": false,
    "pagesPerSheet": 1,
    "mediaSize": {
      "width_microns": 215900,
      "height_microns": 279400,
      "imageable_area_right_microns": 215900,
      "imageable_area_top_microns": 279400
    },
    "scalingType": %d,
    "isFirstRequest": true,
    "previewUIID": %d,
    "requestID": %d,
    "title": "%s",
    "previewModifiable": %s,
    "url": "%s"
    })";

    std::optional<int32_t> print_preview_ui_id;
    int request_id = -1;
    auto on_complete =
        base::BindLambdaForTesting([&print_preview_ui_id, &request_id, this]() {
          print_preview_ui_id = GetPrintPreviewUIId();
          auto request_id_map =
              printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap();
          request_id = request_id_map[*print_preview_ui_id];
        });

    auto print_render_frame = SetupPrintPreviewTest(
        mime_type,
        MockPrintPreviewPrintRenderFrame::ExpectedError::kPrintPreviewFailed,
        std::move(on_complete));

    base::test::TestFuture<
        base::expected<std::vector<std::vector<uint8_t>>, std::string>>
        future;
    pp_extractor_->CaptureImages(future.GetCallback());
    auto result = future.Take();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "PrintPreviewFailed");

    EXPECT_TRUE(
        printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap().empty());
    EXPECT_FALSE(GetPrintPreviewUIId());

    ASSERT_TRUE(print_preview_ui_id);
    EXPECT_EQ(
        print_render_frame->GetSettings(),
        base::test::ParseJsonDict(absl::StrFormat(
            kSettingTemplate,
            static_cast<int>(printing::mojom::MarginType::kDefaultMargins),
            static_cast<int>(printing::mojom::ColorModel::kColor),
            printing::kDefaultPdfDpi, printing::kDefaultPdfDpi,
            static_cast<int>(printing::mojom::DuplexMode::kSimplex),
            static_cast<int>(printing::mojom::PrinterType::kPdf),
            static_cast<int>(printing::ScalingType::DEFAULT),
            *print_preview_ui_id, request_id,
            base::UTF16ToUTF8(web_contents()->GetTitle()),
            base::ToString(expect_preview_modifiable),
            web_contents()->GetLastCommittedURL().spec())));
  }

  // Test for all possible PrintRenderFrame errors
  void RunErrorTest(
      const std::string& mime_type,
      MockPrintPreviewPrintRenderFrame::ExpectedError expected_error,
      const std::string& expected_error_message,
      const base::Location& location = FROM_HERE) {
    SCOPED_TRACE(location.ToString());

    auto print_render_frame = SetupPrintPreviewTest(mime_type, expected_error);

    base::test::TestFuture<
        base::expected<std::vector<std::vector<uint8_t>>, std::string>>
        future;
    pp_extractor_->CaptureImages(future.GetCallback());
    auto result = future.Take();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), expected_error_message);

    EXPECT_TRUE(
        printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap().empty());
    EXPECT_FALSE(GetPrintPreviewUIId());
  }

  void SimulateFullFlow(bool extractor_error) {
    auto full_pdf_region = CreatePageRegion(64).region;
    internal_extractor()->SetPreviewPageImageExtractorForTesting(
        std::make_unique<MockPreviewPageImageExtractor>(
            full_pdf_region.Duplicate(), extractor_error));

    // Simulate page composition
    internal_extractor()->OnCompositePdfPageDone(
        0, 0, 0, printing::mojom::PrintCompositor::Status::kSuccess,
        CreatePageRegion(8).region);
    internal_extractor()->OnCompositePdfPageDone(
        1, 0, 0, printing::mojom::PrintCompositor::Status::kSuccess,
        CreatePageRegion(16).region);
    internal_extractor()->OnCompositeToPdfDone(
        0, 0, printing::mojom::PrintCompositor::Status::kSuccess,
        full_pdf_region.Duplicate());
  }

  // Test all possible flows with mocked PreviewPageImageExtractor that returns
  // fixed successful results.
  template <typename T>
  void RunTestCase(base::test::TestFuture<T>& future,
                   const std::string& mime_type,
                   const std::string& expected_error_msg,
                   bool extractor_error,
                   bool simulate_partial_composition) {
    const bool expect_error = !expected_error_msg.empty();
    if (!expect_error || extractor_error) {
      SimulateFullFlow(extractor_error);
    } else if (simulate_partial_composition) {
      // Simulate partial composition without setting preview data in
      // OnCompositeToPdfDone
      internal_extractor()->OnCompositePdfPageDone(
          0, 0, 0, printing::mojom::PrintCompositor::Status::kSuccess,
          CreatePageRegion(8).region);
      internal_extractor()->OnCompositePdfPageDone(
          1, 0, 0, printing::mojom::PrintCompositor::Status::kSuccess,
          CreatePageRegion(16).region);
      internal_extractor()->OnCompositeToPdfDone(
          0, 0, printing::mojom::PrintCompositor::Status::kCompositingFailure,
          CreatePageRegion(32).region);
    } else {
      // Missing preview data
      internal_extractor()->OnCompositeToPdfDone(
          0, 0, printing::mojom::PrintCompositor::Status::kCompositingFailure,
          CreatePageRegion(32).region);
    }

    auto result = future.Take();
    if (expect_error) {
      ASSERT_FALSE(result.has_value())
          << "Expected error for mime_type=" << mime_type
          << ", partial_composition=" << simulate_partial_composition;
      EXPECT_EQ(result.error(), expected_error_msg);
    } else {
      ASSERT_TRUE(result.has_value())
          << "Expected success for mime_type=" << mime_type;
      if constexpr (std::is_same_v<T, ImageResult>) {
        const std::vector<std::vector<uint8_t>> expected = {{0xde, 0xad},
                                                            {0xbe, 0xef}};
        EXPECT_EQ(result.value(), expected);
      } else {
        EXPECT_EQ(result.value(), std::string("extracted text"));
      }
    }
  }

  PrintPreviewExtractorInternal* internal_extractor() {
    return static_cast<PrintPreviewExtractorInternal*>(
        pp_extractor_->extractor_.get());
  }

 protected:
  std::unique_ptr<PrintPreviewExtractor> pp_extractor_;
};

TEST_F(PrintPreviewExtractorTest, CaptureImagesWithNonPdf) {
  content::WebContentsTester::For(web_contents())
      ->SetMainFrameMimeType("text/html");

  auto print_render_frame = SetupPrintPreviewTest(
      "text/html",
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrintPreviewFailed);

  base::test::TestFuture<
      base::expected<std::vector<std::vector<uint8_t>>, std::string>>
      future;
  pp_extractor_->CaptureImages(future.GetCallback());
  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "PrintPreviewFailed");
}

TEST_F(PrintPreviewExtractorTest, PrintSettings) {
  // Test with non-PDF content
  RunPrintSettingsTest("text/html", true);

  // Test with PDF content using CaptureImages()
  RunPrintSettingsTest("application/pdf", false);
}

TEST_F(PrintPreviewExtractorTest, Errors) {
  // Test CaptureImages() with various errors
  RunErrorTest(
      "text/html",
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrintPreviewFailed,
      "PrintPreviewFailed");
  RunErrorTest(
      "text/html",
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrintPreviewCanceled,
      "PrintPreviewCancelled");
  RunErrorTest(
      "text/html",
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrinterSettingsInvalid,
      "PrinterSettingsInvalid");

  RunErrorTest(
      "application/pdf",
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrintPreviewFailed,
      "PrintPreviewFailed");
  RunErrorTest(
      "application/pdf",
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrintPreviewCanceled,
      "PrintPreviewCancelled");
  RunErrorTest(
      "application/pdf",
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrinterSettingsInvalid,
      "PrinterSettingsInvalid");

  // Test disabled print preview
  profile()->GetPrefs()->SetBoolean(::prefs::kPrintPreviewDisabled, true);
  RunErrorTest("application/pdf",
               MockPrintPreviewPrintRenderFrame::ExpectedError::kNone,
               "Print preview is disabled");
  RunErrorTest("text/html",
               MockPrintPreviewPrintRenderFrame::ExpectedError::kNone,
               "Print preview is disabled");
}

TEST_F(PrintPreviewExtractorTest, PrintPreviewData) {
  struct TestParams {
    std::string mime_type;
    std::string expected_error_msg;  // Empty string means success
    bool extractor_error;
    bool simulate_partial_composition;
  };

  const TestParams kTestCases[] = {
      // Missing preview data cases - error on OnCompositeToPdfDone
      {"text/html", "Failed to get preview data", false, false},
      {"application/pdf", "Failed to get preview data", false, false},

      // Missing preview data cases - missing OnCompositePdfPageDone and error
      // on OnCompositeToPdfDone
      {"text/html", "Failed to get preview data", false, true},
      {"application/pdf", "Failed to get preview data", false, true},

      // Successful extraction cases
      {"text/html", "", false, false},
      {"application/pdf", "", false, false},

      // Extraction error cases
      {"text/html", "PreviewPageImageExtractor error", true, false},
      {"application/pdf", "PreviewPageImageExtractor error", true, false},
  };

  for (const auto& test_case : kTestCases) {
    auto print_render_frame = SetupPrintPreviewTest(
        test_case.mime_type,
        MockPrintPreviewPrintRenderFrame::ExpectedError::kNone);

    base::test::TestFuture<ImageResult> future;
    pp_extractor_->CaptureImages(future.GetCallback());
    RunTestCase(future, test_case.mime_type, test_case.expected_error_msg,
                test_case.extractor_error,
                test_case.simulate_partial_composition);
  }
}

class PreviewPageImageExtractorTest : public testing::Test {
 public:
  PreviewPageImageExtractorTest() = default;
  ~PreviewPageImageExtractorTest() override = default;

  void SetUp() override {
    extractor_ = std::make_unique<PreviewPageImageExtractor>();
    converter_ = std::make_unique<MockPdfToBitmapConverter>();
    extractor()->BindForTesting(converter()->Bind());
  }

  PreviewPageImageExtractor* extractor() { return extractor_.get(); }

  MockPdfToBitmapConverter* converter() { return converter_.get(); }

  // Testing early errors
  void RunErrorTest(const std::string& expected_error,
                    const base::Location& location = FROM_HERE) {
    SCOPED_TRACE(location.ToString());
    {
      base::test::TestFuture<ImageResult> future;
      extractor()->StartExtract(CreatePageRegion(50).region,
                                future.GetCallback(), std::nullopt);
      auto result = future.Take();
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error(), expected_error);
    }
    {
      base::test::TestFuture<ImageResult> future;
      extractor()->StartExtract(CreatePageRegion(50).region,
                                future.GetCallback(), std::nullopt);
      auto result = future.Take();
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error(), expected_error);
    }
  }

  // Testing image capturing logic with page count and make sure processed
  // images are not empty
  void RunCaptureImageTest(uint32_t expected_page_count,
                           const base::Location& location = FROM_HERE) {
    SCOPED_TRACE(location.ToString());
    converter()->SetExpectedPageCount(expected_page_count);
    base::test::TestFuture<ImageResult> future;
    extractor()->StartExtract(CreatePageRegion(50).region, future.GetCallback(),
                              std::nullopt);
    auto result = future.Take();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), expected_page_count);
    EXPECT_TRUE(std::ranges::any_of(result.value(), [](const auto& image) {
      return image.empty() ? false : true;
    }));
  }

 private:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<PreviewPageImageExtractor> extractor_;
  std::unique_ptr<MockPdfToBitmapConverter> converter_;
};

TEST_F(PreviewPageImageExtractorTest, GetPdfPageCountError) {
  converter()->SetExpectedPageCount(std::nullopt);
  RunErrorTest("Failed to get page count");
}

TEST_F(PreviewPageImageExtractorTest, GetBitmapError) {
  converter()->SetExpectedPageCount(1);
  converter()->SetExpectedEmptyBitmap(true);
  RunErrorTest("Invalid bitmap");

  converter()->SetExpectedPageCount(3);
  RunErrorTest("Invalid bitmap");
}

TEST_F(PreviewPageImageExtractorTest, CaptureImages) {
  converter()->SetExpectedEmptyBitmap(false);

  // Test with single page
  RunCaptureImageTest(1);

  // Test with multiple pages
  RunCaptureImageTest(3);

  // Test with many pages
  RunCaptureImageTest(20);

  // Test with even more pages
  RunCaptureImageTest(25);
}

}  // namespace ai_chat
