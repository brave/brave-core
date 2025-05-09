// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/ai_chat/print_preview_extractor.h"

#include <memory>
#include <utility>

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
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
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

namespace ai_chat {

namespace {

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
  void PrintPreview(base::Value::Dict settings) override {
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

  const base::Value::Dict& GetSettings() { return settings_; }

  void SetPrintPreviewCalledClosure(base::OnceClosure closure) {
    closure_ = std::move(closure);
  }

 private:
  base::OnceClosure closure_;
  base::Value::Dict settings_;
  ExpectedError expected_error_ = ExpectedError::kNone;

  mojo::AssociatedRemote<printing::mojom::PrintPreviewUI> preview_ui_;
  mojo::AssociatedReceiver<printing::mojom::PrintRenderFrame> receiver_{this};
};

}  // namespace
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
    pp_extractor_ = std::make_unique<PrintPreviewExtractor>(web_contents());
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

  void RunPrintSettingsTest(const std::string& mime_type,
                            bool use_capture_pdf,
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

    if (use_capture_pdf) {
      base::test::TestFuture<
          base::expected<std::vector<std::vector<uint8_t>>, std::string>>
          future;
      pp_extractor_->CapturePdf(future.GetCallback());
      auto result = future.Take();
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error(), "PrintPreviewFailed");
    } else {
      base::test::TestFuture<base::expected<std::string, std::string>> future;
      pp_extractor_->Extract(future.GetCallback());
      auto result = future.Take();
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error(), "PrintPreviewFailed");
    }

    EXPECT_TRUE(
        printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap().empty());
    EXPECT_FALSE(GetPrintPreviewUIId());

    ASSERT_TRUE(print_preview_ui_id);
    EXPECT_EQ(
        print_render_frame->GetSettings(),
        base::test::ParseJsonDict(base::StringPrintf(
            kSettingTemplate,
            static_cast<int>(printing::mojom::MarginType::kDefaultMargins),
            static_cast<int>(printing::mojom::ColorModel::kColor),
            printing::kDefaultPdfDpi, printing::kDefaultPdfDpi,
            static_cast<int>(printing::mojom::DuplexMode::kSimplex),
            static_cast<int>(printing::mojom::PrinterType::kPdf),
            static_cast<int>(printing::ScalingType::DEFAULT),
            *print_preview_ui_id, request_id,
            base::UTF16ToUTF8(web_contents()->GetTitle()),
            expect_preview_modifiable ? "true" : "false",
            web_contents()->GetLastCommittedURL().spec())));
  }

  void RunErrorTest(
      const std::string& mime_type,
      bool use_capture_pdf,
      MockPrintPreviewPrintRenderFrame::ExpectedError expected_error,
      const std::string& expected_error_message,
      const base::Location& location = FROM_HERE) {
    SCOPED_TRACE(location.ToString());

    auto print_render_frame = SetupPrintPreviewTest(mime_type, expected_error);

    if (use_capture_pdf) {
      base::test::TestFuture<
          base::expected<std::vector<std::vector<uint8_t>>, std::string>>
          future;
      pp_extractor_->CapturePdf(future.GetCallback());
      auto result = future.Take();
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error(), expected_error_message);
    } else {
      base::test::TestFuture<base::expected<std::string, std::string>> future;
      pp_extractor_->Extract(future.GetCallback());
      auto result = future.Take();
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error(), expected_error_message);
    }

    EXPECT_TRUE(
        printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap().empty());
    EXPECT_FALSE(GetPrintPreviewUIId());
  }

 protected:
  std::unique_ptr<PrintPreviewExtractor> pp_extractor_;
};

TEST_F(PrintPreviewExtractorTest, CapturePdfWithNotPdf) {
  content::WebContentsTester::For(web_contents())
      ->SetMainFrameMimeType("text/html");
  base::test::TestFuture<
      base::expected<std::vector<std::vector<uint8_t>>, std::string>>
      future;
  pp_extractor_->CapturePdf(future.GetCallback());
  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Not pdf content");
}

TEST_F(PrintPreviewExtractorTest, PrintSettings) {
  // Test with non-PDF content using Extract()
  RunPrintSettingsTest("text/html", false, true);

  // Test with PDF content using Extract()
  RunPrintSettingsTest("application/pdf", false, false);

  // Test with PDF content using CapturePdf()
  RunPrintSettingsTest("application/pdf", true, false);
}

TEST_F(PrintPreviewExtractorTest, Errors) {
  // Test Extract() with various errors
  RunErrorTest(
      "text/html", false,
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrintPreviewFailed,
      "PrintPreviewFailed");
  RunErrorTest(
      "text/html", false,
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrintPreviewCanceled,
      "PrintPreviewCancelled");
  RunErrorTest(
      "text/html", false,
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrinterSettingsInvalid,
      "PrinterSettingsInvalid");

  // Test CapturePdf() with various errors
  RunErrorTest(
      "application/pdf", true,
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrintPreviewFailed,
      "PrintPreviewFailed");
  RunErrorTest(
      "application/pdf", true,
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrintPreviewCanceled,
      "PrintPreviewCancelled");
  RunErrorTest(
      "application/pdf", true,
      MockPrintPreviewPrintRenderFrame::ExpectedError::kPrinterSettingsInvalid,
      "PrinterSettingsInvalid");

  // Test disabled print preview
  profile()->GetPrefs()->SetBoolean(::prefs::kPrintPreviewDisabled, true);
  RunErrorTest("application/pdf", true,
               MockPrintPreviewPrintRenderFrame::ExpectedError::kNone,
               "Print preview is disabled");
  RunErrorTest("text/html", false,
               MockPrintPreviewPrintRenderFrame::ExpectedError::kNone,
               "Print preview is disabled");
  RunErrorTest("application/pdf", false,
               MockPrintPreviewPrintRenderFrame::ExpectedError::kNone,
               "Print preview is disabled");
}

}  // namespace ai_chat
