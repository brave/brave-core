// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/pdf_text_helper.h"

#include <optional>
#include <string>

#include "base/test/test_future.h"
#include "components/pdf/browser/pdf_document_helper.h"
#include "components/pdf/browser/pdf_document_helper_client.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "pdf/mojom/pdf.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

class FakePdfListener : public pdf::mojom::PdfListener {
 public:
  FakePdfListener() = default;
  ~FakePdfListener() override = default;

  MOCK_METHOD(void, SetCaretPosition, (const gfx::PointF&), (override));
  MOCK_METHOD(void, MoveRangeSelectionExtent, (const gfx::PointF&), (override));
  MOCK_METHOD(void,
              SetSelectionBounds,
              (const gfx::PointF&, const gfx::PointF&),
              (override));
  MOCK_METHOD(void, GetPdfBytes, (uint32_t, GetPdfBytesCallback), (override));
  MOCK_METHOD(void, GetPageText, (int32_t, GetPageTextCallback), (override));
  MOCK_METHOD(void,
              GetMostVisiblePageIndex,
              (GetMostVisiblePageIndexCallback),
              (override));
  MOCK_METHOD(void,
              GetSaveDataBufferHandlerForDrive,
              (pdf::mojom::SaveRequestType,
               GetSaveDataBufferHandlerForDriveCallback),
              (override));
};

class TestPdfDocumentHelperClient : public pdf::PDFDocumentHelperClient {
 public:
  TestPdfDocumentHelperClient() = default;
  ~TestPdfDocumentHelperClient() override = default;

 private:
  void OnDidScroll(const gfx::SelectionBound&,
                   const gfx::SelectionBound&) override {}
};

}  // namespace

class PdfTextHelperTest : public content::RenderViewHostTestHarness {
 protected:
  void SetUpPdfHelper() {
    auto* rfh = web_contents()->GetPrimaryMainFrame();
    pdf::PDFDocumentHelper::CreateForCurrentDocument(
        rfh, std::make_unique<TestPdfDocumentHelperClient>());
    auto* helper = pdf::PDFDocumentHelper::GetForCurrentDocument(rfh);
    helper->SetListener(listener_receiver_.BindNewPipeAndPassRemote());
    helper->OnDocumentLoadComplete();
    // Flush mojo so the listener remote is connected before tests run.
    listener_receiver_.FlushForTesting();
  }

  testing::NiceMock<FakePdfListener>& listener() { return listener_; }

 private:
  testing::NiceMock<FakePdfListener> listener_;
  mojo::Receiver<pdf::mojom::PdfListener> listener_receiver_{&listener_};
};

TEST_F(PdfTextHelperTest, NoPdfDocumentHelper_ReturnsNullopt) {
  base::test::TestFuture<std::optional<std::string>> future;
  ExtractTextFromLoadedPdf(web_contents(), future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());
}

TEST_F(PdfTextHelperTest, SinglePage_ExtractsText) {
  SetUpPdfHelper();

  ON_CALL(listener(), GetPdfBytes)
      .WillByDefault([](uint32_t,
                        pdf::mojom::PdfListener::GetPdfBytesCallback cb) {
        std::move(cb).Run(pdf::mojom::PdfListener::GetPdfBytesStatus::kSuccess,
                          {},
                          /*page_count=*/1);
      });
  ON_CALL(listener(), GetPageText)
      .WillByDefault(
          [](int32_t, pdf::mojom::PdfListener::GetPageTextCallback cb) {
            std::move(cb).Run(u"Hello from PDF");
          });

  base::test::TestFuture<std::optional<std::string>> future;
  ExtractTextFromLoadedPdf(web_contents(), future.GetCallback());

  auto result = future.Get();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Hello from PDF");
}

TEST_F(PdfTextHelperTest, MultiPage_JoinsWithNewline) {
  SetUpPdfHelper();

  ON_CALL(listener(), GetPdfBytes)
      .WillByDefault([](uint32_t,
                        pdf::mojom::PdfListener::GetPdfBytesCallback cb) {
        std::move(cb).Run(pdf::mojom::PdfListener::GetPdfBytesStatus::kSuccess,
                          {},
                          /*page_count=*/3);
      });
  ON_CALL(listener(), GetPageText)
      .WillByDefault([](int32_t page_index,
                        pdf::mojom::PdfListener::GetPageTextCallback cb) {
        switch (page_index) {
          case 0:
            std::move(cb).Run(u"Page one");
            break;
          case 1:
            std::move(cb).Run(u"Page two");
            break;
          case 2:
            std::move(cb).Run(u"Page three");
            break;
        }
      });

  base::test::TestFuture<std::optional<std::string>> future;
  ExtractTextFromLoadedPdf(web_contents(), future.GetCallback());

  auto result = future.Get();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Page one\nPage two\nPage three");
}

TEST_F(PdfTextHelperTest, GetPdfBytesFailed_ReturnsNullopt) {
  SetUpPdfHelper();

  ON_CALL(listener(), GetPdfBytes)
      .WillByDefault([](uint32_t,
                        pdf::mojom::PdfListener::GetPdfBytesCallback cb) {
        std::move(cb).Run(pdf::mojom::PdfListener::GetPdfBytesStatus::kFailed,
                          {},
                          /*page_count=*/0);
      });

  base::test::TestFuture<std::optional<std::string>> future;
  ExtractTextFromLoadedPdf(web_contents(), future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());
}

TEST_F(PdfTextHelperTest, ZeroPages_ReturnsNullopt) {
  SetUpPdfHelper();

  ON_CALL(listener(), GetPdfBytes)
      .WillByDefault([](uint32_t,
                        pdf::mojom::PdfListener::GetPdfBytesCallback cb) {
        std::move(cb).Run(pdf::mojom::PdfListener::GetPdfBytesStatus::kSuccess,
                          {},
                          /*page_count=*/0);
      });

  base::test::TestFuture<std::optional<std::string>> future;
  ExtractTextFromLoadedPdf(web_contents(), future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());
}

}  // namespace ai_chat
