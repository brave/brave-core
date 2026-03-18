// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/upload_file_helper.h"

#include <algorithm>

#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ref_counted_memory.h"
#include "base/scoped_observation.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "chrome/browser/ui/select_file_policy/chrome_select_file_policy.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/file_system_chooser_test_helpers.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace ai_chat {

namespace {

// Test data for PDF files
constexpr uint8_t kSamplePdf[] = {
    0x25, 0x50, 0x44, 0x46, 0x2d, 0x31, 0x2e, 0x34, 0x0a, 0x25, 0xc7, 0xec,
    0x8f, 0xa2, 0x0a, 0x31, 0x20, 0x30, 0x20, 0x6f, 0x62, 0x6a, 0x0a, 0x3c,
    0x3c, 0x20, 0x2f, 0x54, 0x79, 0x70, 0x65, 0x20, 0x2f, 0x43, 0x61, 0x74,
    0x61, 0x6c, 0x6f, 0x67, 0x20, 0x2f, 0x50, 0x61, 0x67, 0x65, 0x73, 0x20,
    0x32, 0x20, 0x30, 0x20, 0x52, 0x20, 0x2f, 0x4f, 0x75, 0x74, 0x6c, 0x69,
    0x6e, 0x65, 0x73, 0x20, 0x33, 0x20, 0x30, 0x20, 0x52, 0x20, 0x2f, 0x4d,
    0x65, 0x74, 0x61, 0x64, 0x61, 0x74, 0x61, 0x20, 0x34, 0x20, 0x30, 0x20,
    0x52, 0x20, 0x3e, 0x3e, 0x0a, 0x65, 0x6e, 0x64, 0x6f, 0x62, 0x6a, 0x0a,
    0x78, 0x72, 0x65, 0x66, 0x0a, 0x30, 0x20, 0x35, 0x0a, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x20, 0x6e, 0x0a, 0x0a, 0x74, 0x72,
    0x61, 0x69, 0x6c, 0x65, 0x72, 0x0a, 0x3c, 0x3c, 0x20, 0x2f, 0x53, 0x69,
    0x7a, 0x65, 0x20, 0x35, 0x20, 0x2f, 0x52, 0x6f, 0x6f, 0x74, 0x20, 0x31,
    0x20, 0x30, 0x20, 0x52, 0x20, 0x2f, 0x49, 0x6e, 0x66, 0x6f, 0x20, 0x34,
    0x20, 0x30, 0x20, 0x52, 0x20, 0x3e, 0x3e, 0x0a, 0x73, 0x74, 0x61, 0x72,
    0x74, 0x78, 0x72, 0x65, 0x66, 0x0a, 0x31, 0x32, 0x38, 0x0a, 0x25, 0x25,
    0x45, 0x4f, 0x46, 0x0a};

// Android returns just the basename as the UploadedFile filename since
// ReadSelectedFile uses display_name there (info.path() is a content:// URI).
std::string ExpectedFilename(const base::FilePath& path) {
#if BUILDFLAG(IS_ANDROID)
  return path.BaseName().AsUTF8Unsafe();
#else
  return path.AsUTF8Unsafe();
#endif
}

class MockObserver : UploadFileHelper::Observer {
 public:
  explicit MockObserver(UploadFileHelper* helper) { obs_.Observe(helper); }
  ~MockObserver() override = default;

  MOCK_METHOD(void, OnFilesSelected, (), (override));

 private:
  base::ScopedObservation<UploadFileHelper, UploadFileHelper::Observer> obs_{
      this};
};

}  // namespace
class UploadFileHelperTest : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    auto* profile = Profile::FromBrowserContext(browser_context());
#if BUILDFLAG(IS_ANDROID)
    TestingBrowserProcess::GetGlobal()
        ->GetTestingLocalState()
        ->registry()
        ->RegisterBooleanPref(prefs::kAllowFileSelectionDialogs, true);
#endif
    // To fulfill ChromeSelectFilePolicy::CanOpenSelectFileDialog()
    TestingBrowserProcess::GetGlobal()->GetTestingLocalState()->SetBoolean(
        prefs::kAllowFileSelectionDialogs, true);
    file_helper_ = std::make_unique<UploadFileHelper>(web_contents(), profile);
  }

  std::optional<std::vector<mojom::UploadedFilePtr>> UploadFileSync() {
    base::test::TestFuture<std::optional<std::vector<mojom::UploadedFilePtr>>>
        future;
    file_helper_->UploadFile(
        std::make_unique<ChromeSelectFilePolicy>(web_contents()),
#if BUILDFLAG(IS_ANDROID)
        false,
#endif
        future.GetCallback());
    return future.Take();
  }

  void TearDown() override {
    file_helper_.reset();
    content::RenderViewHostTestHarness::TearDown();
    ui::SelectFileDialog::SetFactory(nullptr);
    ASSERT_TRUE(temp_dir_.Delete());
  }

  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override {
    return std::make_unique<TestingProfile>();
  }

 protected:
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<UploadFileHelper> file_helper_;
  // Must persist throughout TearDown().
  content::SelectFileDialogParams dialog_params_;
};

TEST_F(UploadFileHelperTest, AcceptedFileExtensions) {
  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::CancellingSelectFileDialogFactory>(
          &dialog_params_));
  // This also test cancel selection result
  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(0);
  EXPECT_FALSE(UploadFileSync());
  EXPECT_EQ(dialog_params_.type, ui::SelectFileDialog::SELECT_OPEN_MULTI_FILE);
  // No extension filtering — all files are accepted, matching how Chromium's
  // Browser::OpenFile works. The renderer handles MIME sniffing.
  ASSERT_TRUE(dialog_params_.file_types);
  EXPECT_TRUE(dialog_params_.file_types->extensions.empty());
#if BUILDFLAG(IS_ANDROID)
  // Android doesn't support view-source for text file extraction, so only
  // image and PDF MIME types are accepted.
  EXPECT_THAT(dialog_params_.accept_types,
              testing::UnorderedElementsAre(u"image/png", u"image/jpeg",
                                            u"image/webp", u"application/pdf"));
#endif
}

TEST_F(UploadFileHelperTest, ImageRead) {
  data_decoder::test::InProcessDataDecoder data_decoder;
  base::FilePath path = temp_dir_.GetPath().AppendASCII("not_png.png");
  ASSERT_TRUE(base::WriteFile(path, "1234"));
  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{path}));
  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(1);
  EXPECT_FALSE(UploadFileSync());
  testing::Mock::VerifyAndClearExpectations(&observer);

  base::FilePath path2 = temp_dir_.GetPath().AppendASCII("empty.png");
  ASSERT_TRUE(base::WriteFile(path2, base::span<uint8_t>()));
  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{path2}));
  EXPECT_CALL(observer, OnFilesSelected).Times(1);
  EXPECT_FALSE(UploadFileSync());
  testing::Mock::VerifyAndClearExpectations(&observer);

  constexpr uint8_t kSamplePng[] = {
      0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00,
      0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
      0x00, 0x01, 0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53, 0xde,
      0x00, 0x00, 0x00, 0x10, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x62,
      0x5a, 0xc4, 0x5e, 0x08, 0x08, 0x00, 0x00, 0xff, 0xff, 0x02, 0x71,
      0x01, 0x1d, 0xcd, 0xd0, 0xd6, 0x62, 0x00, 0x00, 0x00, 0x00, 0x49,
      0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};
  auto sample_bitmap = gfx::PNGCodec::Decode(kSamplePng);
  base::FilePath path3 = temp_dir_.GetPath().AppendASCII("sample_png.png");
  ASSERT_TRUE(base::WriteFile(path3, kSamplePng));
  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{path3}));
  EXPECT_CALL(observer, OnFilesSelected).Times(1);
  auto sample_result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);
  ASSERT_TRUE(sample_result);
  ASSERT_EQ(1u, sample_result->size());
  EXPECT_EQ((*sample_result)[0]->filename, ExpectedFilename(path3));
  EXPECT_EQ((*sample_result)[0]->filesize, (*sample_result)[0]->data.size());
  EXPECT_EQ((*sample_result)[0]->type, mojom::UploadedFileType::kImage);
  auto encoded_bitmap = gfx::PNGCodec::Decode((*sample_result)[0]->data);
  EXPECT_TRUE(gfx::test::AreBitmapsClose(sample_bitmap, encoded_bitmap, 1));
  // Check dimensions are the same.
  EXPECT_EQ(sample_bitmap.width(), encoded_bitmap.width());
  EXPECT_EQ(sample_bitmap.height(), encoded_bitmap.height());

  // Large image will be scaled into 1024x768
  auto large_png_bytes = gfx::test::CreatePNGBytes(2048);
  base::FilePath path4 = temp_dir_.GetPath().AppendASCII("large_png.png");
  ASSERT_TRUE(base::WriteFile(path4, base::span(*large_png_bytes)));
  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{path4}));
  EXPECT_CALL(observer, OnFilesSelected).Times(1);
  auto large_result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);
  ASSERT_TRUE(large_result);
  ASSERT_EQ(1u, large_result->size());
  EXPECT_EQ((*large_result)[0]->filename, ExpectedFilename(path4));
  EXPECT_EQ((*large_result)[0]->filesize, (*large_result)[0]->data.size());
  EXPECT_EQ((*large_result)[0]->type, mojom::UploadedFileType::kImage);
  EXPECT_LE((*large_result)[0]->filesize, large_png_bytes->size());
  encoded_bitmap = gfx::PNGCodec::Decode((*large_result)[0]->data);
  EXPECT_EQ(1024, encoded_bitmap.width());
  EXPECT_EQ(768, encoded_bitmap.height());

  // multiple image selection
  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{path3, path4}));

  EXPECT_CALL(observer, OnFilesSelected).Times(1);
  auto result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);

  ASSERT_TRUE(result);
  ASSERT_EQ(2u, result->size());

  EXPECT_EQ((*result)[0]->filename, ExpectedFilename(path3));
  EXPECT_EQ((*result)[0]->filesize, (*result)[0]->data.size());
  EXPECT_EQ((*result)[0]->type, mojom::UploadedFileType::kImage);
  auto encoded_bitmap1 = gfx::PNGCodec::Decode((*result)[0]->data);
  EXPECT_TRUE(gfx::test::AreBitmapsClose(sample_bitmap, encoded_bitmap1, 1));
  EXPECT_EQ(sample_bitmap.width(), encoded_bitmap1.width());
  EXPECT_EQ(sample_bitmap.height(), encoded_bitmap1.height());

  EXPECT_EQ((*result)[1]->filename, ExpectedFilename(path4));
  EXPECT_EQ((*result)[1]->filesize, (*result)[1]->data.size());
  EXPECT_EQ((*result)[1]->type, mojom::UploadedFileType::kImage);
  auto encoded_bitmap2 = gfx::PNGCodec::Decode((*result)[1]->data);
  EXPECT_EQ(1024, encoded_bitmap2.width());
  EXPECT_EQ(768, encoded_bitmap2.height());
}

TEST_F(UploadFileHelperTest, PdfFileHandling) {
  // Test PDF file with valid PDF header

  base::FilePath pdf_path = temp_dir_.GetPath().AppendASCII("sample.pdf");
  ASSERT_TRUE(base::WriteFile(pdf_path, kSamplePdf));

  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{pdf_path}));

  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(1);

  auto result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);

  ASSERT_TRUE(result);
  ASSERT_EQ(1u, result->size());
  EXPECT_EQ((*result)[0]->filename, ExpectedFilename(pdf_path));
  EXPECT_EQ((*result)[0]->filesize, (*result)[0]->data.size());
  EXPECT_EQ((*result)[0]->type, mojom::UploadedFileType::kPdf);

  // Verify the PDF data is returned unchanged (no processing)
  EXPECT_EQ((*result)[0]->data.size(), sizeof(kSamplePdf));
  EXPECT_EQ(base::span((*result)[0]->data), base::span(kSamplePdf));
}

TEST_F(UploadFileHelperTest, PdfFileWithInvalidHeader) {
  // Test file with .pdf extension but invalid content
  constexpr uint8_t kInvalidPdf[] = {
      0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c,
      0x64, 0x21, 0x20, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73,
      0x20, 0x6e, 0x6f, 0x74, 0x20, 0x61, 0x20, 0x50, 0x44, 0x46,
      0x20, 0x66, 0x69, 0x6c, 0x65, 0x2e, 0x0a};

  base::FilePath invalid_pdf_path =
      temp_dir_.GetPath().AppendASCII("invalid.pdf");
  ASSERT_TRUE(base::WriteFile(invalid_pdf_path, kInvalidPdf));

  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{invalid_pdf_path}));

  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(1);

  auto result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);

  // Should return a stub entry (empty data, kText) so the frontend can
  // detect the rejection and show an error.
  ASSERT_TRUE(result);
  ASSERT_EQ(1u, result->size());
  EXPECT_TRUE((*result)[0]->data.empty());
  EXPECT_EQ((*result)[0]->type, mojom::UploadedFileType::kText);
  EXPECT_FALSE((*result)[0]->extracted_text.has_value());
}

TEST_F(UploadFileHelperTest, PdfFileTooSmall) {
  // Test PDF file that's too small to be valid (less than 50 bytes)
  constexpr uint8_t kSmallPdf[] = {0x25, 0x50, 0x44, 0x46, 0x2d,
                                   0x31, 0x2e, 0x34, 0x0a};

  base::FilePath small_pdf_path = temp_dir_.GetPath().AppendASCII("small.pdf");
  ASSERT_TRUE(base::WriteFile(small_pdf_path, kSmallPdf));

  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{small_pdf_path}));

  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(1);

  auto result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);

  // Should return a stub entry for the rejected file.
  ASSERT_TRUE(result);
  ASSERT_EQ(1u, result->size());
  EXPECT_TRUE((*result)[0]->data.empty());
  EXPECT_EQ((*result)[0]->type, mojom::UploadedFileType::kText);
  EXPECT_FALSE((*result)[0]->extracted_text.has_value());
}

TEST_F(UploadFileHelperTest, BinaryFileRejected) {
  // Test that files with known binary MIME types are rejected
  base::FilePath zip_path = temp_dir_.GetPath().AppendASCII("archive.zip");
  ASSERT_TRUE(base::WriteFile(zip_path, "PK\x03\x04 fake zip content"));

  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{zip_path}));

  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(1);

  auto result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);

  // Should return a stub entry for the rejected zip file.
  ASSERT_TRUE(result);
  ASSERT_EQ(1u, result->size());
  EXPECT_TRUE((*result)[0]->data.empty());
  EXPECT_EQ((*result)[0]->type, mojom::UploadedFileType::kText);
  EXPECT_FALSE((*result)[0]->extracted_text.has_value());
}

TEST_F(UploadFileHelperTest, TextFileHandling) {
  // Test text file with known extension
  base::FilePath txt_path = temp_dir_.GetPath().AppendASCII("readme.txt");
  ASSERT_TRUE(base::WriteFile(txt_path, "Hello, world!"));

  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{txt_path}));

  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(1);

  auto result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);

  ASSERT_TRUE(result);
  ASSERT_EQ(1u, result->size());
  EXPECT_EQ((*result)[0]->filename, ExpectedFilename(txt_path));
  EXPECT_EQ((*result)[0]->type, mojom::UploadedFileType::kText);
  EXPECT_EQ((*result)[0]->data.size(), 13u);
}

TEST_F(UploadFileHelperTest, TextFileWithoutExtension) {
  // Test file without extension — should be treated as text
  base::FilePath no_ext_path = temp_dir_.GetPath().AppendASCII("textfile");
  ASSERT_TRUE(base::WriteFile(no_ext_path, "some text content"));

  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{no_ext_path}));

  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(1);

  auto result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);

  ASSERT_TRUE(result);
  ASSERT_EQ(1u, result->size());
  EXPECT_EQ((*result)[0]->type, mojom::UploadedFileType::kText);
}

TEST_F(UploadFileHelperTest, TextFileWithTrailingDot) {
  // Test file ending with a dot — should be treated as text
  base::FilePath dot_path = temp_dir_.GetPath().AppendASCII("file.");
  ASSERT_TRUE(base::WriteFile(dot_path, "trailing dot content"));

  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{dot_path}));

  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(1);

  auto result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);

  ASSERT_TRUE(result);
  ASSERT_EQ(1u, result->size());
  EXPECT_EQ((*result)[0]->type, mojom::UploadedFileType::kText);
}

TEST_F(UploadFileHelperTest, TextFileWithUnknownExtension) {
  // Test file with extension not in Chromium's MIME registry
  base::FilePath diff_path = temp_dir_.GetPath().AppendASCII("changes.diff");
  ASSERT_TRUE(base::WriteFile(diff_path, "--- a/file\n+++ b/file\n"));

  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{diff_path}));

  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(1);

  auto result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);

  ASSERT_TRUE(result);
  ASSERT_EQ(1u, result->size());
  EXPECT_EQ((*result)[0]->type, mojom::UploadedFileType::kText);
}

TEST_F(UploadFileHelperTest, MixedFileTypes) {
  data_decoder::test::InProcessDataDecoder data_decoder;
  // Test uploading both PDF and image files together

  auto png_bytes = gfx::test::CreatePNGBytes(100);
  base::FilePath pdf_path = temp_dir_.GetPath().AppendASCII("document.pdf");
  base::FilePath png_path = temp_dir_.GetPath().AppendASCII("image.png");
  ASSERT_TRUE(base::WriteFile(pdf_path, kSamplePdf));
  ASSERT_TRUE(base::WriteFile(png_path, base::span(*png_bytes)));

  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{pdf_path, png_path}));

  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(1);

  auto result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);

  ASSERT_TRUE(result);
  ASSERT_EQ(2u, result->size());

  EXPECT_EQ((*result)[0]->filename, ExpectedFilename(pdf_path));
  EXPECT_EQ((*result)[0]->type, mojom::UploadedFileType::kPdf);
  EXPECT_EQ((*result)[0]->data.size(), sizeof(kSamplePdf));

  EXPECT_EQ((*result)[1]->filename, ExpectedFilename(png_path));
  EXPECT_EQ((*result)[1]->type, mojom::UploadedFileType::kImage);
  EXPECT_GT((*result)[1]->data.size(), 0u);
}

TEST_F(UploadFileHelperTest, MixedWithUnsupportedFileDropsInvalid) {
  data_decoder::test::InProcessDataDecoder data_decoder;
  // Upload a valid image and text file alongside an unsupported zip file.
  auto png_bytes = gfx::test::CreatePNGBytes(100);
  base::FilePath png_path = temp_dir_.GetPath().AppendASCII("photo.png");
  base::FilePath txt_path = temp_dir_.GetPath().AppendASCII("readme.txt");
  base::FilePath zip_path = temp_dir_.GetPath().AppendASCII("archive.zip");
  ASSERT_TRUE(base::WriteFile(png_path, base::span(*png_bytes)));
  ASSERT_TRUE(base::WriteFile(txt_path, "hello world"));
  ASSERT_TRUE(base::WriteFile(zip_path, "PK\x03\x04 fake zip"));

  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{zip_path, png_path, txt_path}));

  testing::NiceMock<MockObserver> observer(file_helper_.get());
  EXPECT_CALL(observer, OnFilesSelected).Times(1);

  auto result = UploadFileSync();
  testing::Mock::VerifyAndClearExpectations(&observer);

  // All 3 files returned: image and text are valid, zip is a stub.
  ASSERT_TRUE(result);
  ASSERT_EQ(3u, result->size());
  // Barrier callback order is nondeterministic, so check by type.
  EXPECT_TRUE(std::ranges::any_of(*result, [](const auto& f) {
    return f->type == mojom::UploadedFileType::kImage && !f->data.empty();
  }));
  // The text file and zip stub both have kText type. Distinguish by data.
  EXPECT_TRUE(std::ranges::any_of(*result, [](const auto& f) {
    return f->type == mojom::UploadedFileType::kText && !f->data.empty();
  }));
  // Zip stub: kText with empty data
  EXPECT_TRUE(std::ranges::any_of(*result, [](const auto& f) {
    return f->type == mojom::UploadedFileType::kText && f->data.empty();
  }));
}

}  // namespace ai_chat
