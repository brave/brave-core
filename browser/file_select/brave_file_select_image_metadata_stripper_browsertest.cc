/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/file_select/brave_file_select_image_metadata_stripper.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/path_service.h"
#include "base/synchronization/lock.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/thread_annotations.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/shell_dialogs/fake_select_file_dialog.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace brave {

namespace {

// The actual file we will upload that has the FBMD iptc data.
constexpr char kUploadTestFileName[] = "fbmd_test_image.jpg";

constexpr std::string_view kFbmdMarker = "FBMD";

bool ContainsFbmd(std::string_view data) {
  return data.find(kFbmdMarker) != std::string_view::npos;
}

constexpr char kUploadFormPagePath[] = "/upload_form.html";
constexpr char kUploadEndpointPath[] = "/upload";

}  // namespace

class FileSelectImageMetadataStripperBase : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // Observe the temporary files the stripper creates, so cleanup can be
    // verified against the exact paths.
    strip_completed_callback_ = strip_completed_future_.GetCallback();
    // Test-only production code.
    SetStripCompletedCallbackForTesting(&strip_completed_callback_);

    const base::FilePath test_data_dir =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
            .AppendASCII("brave/test/data/image_metadata_stripper");
    fbmd_test_image_path_ = test_data_dir.AppendASCII(kUploadTestFileName);

    // Serve the upload form page from our test-data directory.
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    // To observe the actual uploaded file. This helps to compare if fbmd was
    // stripped or not.
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&FileSelectImageMetadataStripperBase::HandleUpload,
                            base::Unretained(this)));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TearDownOnMainThread() override {
    SetStripCompletedCallbackForTesting(nullptr);

    ui::SelectFileDialog::SetFactory(nullptr);
    InProcessBrowserTest::TearDownOnMainThread();
  }

 protected:
  explicit FileSelectImageMetadataStripperBase(bool strip_metadata) {
    feature_list_.InitWithFeatureState(
        image_metadata_stripper::features::kStripImageMetadataV1,
        strip_metadata);
  }

  // Opens a fresh tab on the upload form page and returns its WebContents.
  content::WebContents* OpenUploadTab() {
    EXPECT_TRUE(
        AddTabAtIndex(1, embedded_test_server()->GetURL(kUploadFormPagePath),
                      ui::PAGE_TRANSITION_TYPED));
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  ui::FakeSelectFileDialog* OpenFilePicker(content::WebContents* web_contents) {
    ui::FakeSelectFileDialog::Factory* factory =
        ui::FakeSelectFileDialog::RegisterFactory();
    // FakeSelectFileDialog unconditionally runs this closure when the picker
    // opens, so it must be non-null
    factory->SetOpenCallback(base::DoNothing());

    // 1. Register listener for change event to drive WaitForFileChange()
    // 2. Click the file input. This should open the file picker dialog which we
    // wait for below.
    EXPECT_TRUE(ExecJs(web_contents, R"(
      window.fileInputChanged = new Promise(resolve => {
        const input = document.getElementById('fileinput');
        input.addEventListener('change',
            () => resolve(input.files.length), {once: true});
      });
      document.getElementById('fileinput').click();
    )"));

    // Waits for the dialog.
    EXPECT_TRUE(base::test::RunUntil(
        [&]() { return factory->GetLastDialog() != nullptr; }));
    return factory->GetLastDialog();
  }

  int WaitForFileSelected(content::WebContents* web_contents) {
    return content::EvalJs(web_contents, "window.fileInputChanged")
        .ExtractInt();
  }

  void SelectFilesInPicker(content::WebContents* web_contents,
                           const std::vector<base::FilePath>& files) {
    OpenFilePicker(web_contents)->CallMultiFilesSelected(files);
    EXPECT_EQ(static_cast<int>(files.size()),
              WaitForFileSelected(web_contents));
  }

  // Opens a fresh tab and selects |files| through the fake dialog.
  content::WebContents* OpenTabAndSelectFiles(
      const std::vector<base::FilePath>& files) {
    content::WebContents* web_contents = OpenUploadTab();
    SelectFilesInPicker(web_contents, files);
    return web_contents;
  }

  // Cancel flow. Opens a fresh tab and cancels the picker.
  content::WebContents* OpenTabAndCancelPicker() {
    content::WebContents* web_contents = OpenUploadTab();
    OpenFilePicker(web_contents)->CallFileSelectionCanceled();
    return web_contents;
  }

  // Re-arms the one-shot strip observer so a subsequent pick can be awaited on
  // |strip_completed_future_| again.
  void ReArmOnStripCompleteObserver() {
    strip_completed_future_.Clear();
    strip_completed_callback_ = strip_completed_future_.GetCallback();
    SetStripCompletedCallbackForTesting(&strip_completed_callback_);
  }

  // Uploads every selected file via multipart/form-data and returns the body
  // the server received.
  std::string UploadFileAndInterceptContent(
      content::WebContents* web_contents) {
    EXPECT_EQ(true, content::EvalJs(web_contents, R"(
      (async () => {
        const data = new FormData();
        for (const file of document.getElementById('fileinput').files) {
          data.append('file', file);
        }
        const resp = await fetch('/upload', {method: 'POST', body: data});
        return resp.ok;
      })()
    )")
                        .ExtractBool());

    base::AutoLock lock(lock_);
    return uploaded_body_;
  }

  bool PathExists(const base::FilePath& path) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return base::PathExists(path);
  }

  base::FilePath fbmd_test_image_path_;
  // Provides the callback for SetStripCompletedCallbackForTesting to intercept
  // the temporary files to test clean-up behaviour.
  base::test::TestFuture<std::vector<base::FilePath>> strip_completed_future_;

 private:
  // This intercepts the uploaded image and converts its contents to text to
  // later test for the existence of FBMD makers.
  std::unique_ptr<net::test_server::HttpResponse> HandleUpload(
      const net::test_server::HttpRequest& request) {
    if (request.relative_url != kUploadEndpointPath) {
      return nullptr;
    }
    // Gaurd to prevent this callback which runs on server's i/o thread
    // accessing |uploaded_body_| at the same time as the UI thread where the
    // test is running.
    {
      base::AutoLock lock(lock_);
      uploaded_body_ = request.content;
    }
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_content_type("text/plain");
    response->set_content("ok");
    return response;
  }

  // The callback is owned here so it outlives the async strip that runs it.
  base::OnceCallback<void(std::vector<base::FilePath>)>
      strip_completed_callback_;

  // This lock guard is used to prevent cases where |uploaded_body_| could be
  // mutated on the IO thread and gets read on UI thread at the same time.
  // See `RegisterRequestHandler` documentation in embedded_test_server.h
  base::Lock lock_;
  std::string uploaded_body_ GUARDED_BY(lock_);

  base::test::ScopedFeatureList feature_list_;
};

// Stripping enabled fixture.
class FileSelectImageMetadataStripperBrowserTest
    : public FileSelectImageMetadataStripperBase {
 public:
  FileSelectImageMetadataStripperBrowserTest()
      : FileSelectImageMetadataStripperBase(
            /*strip_metadata=*/true) {}
};

IN_PROC_BROWSER_TEST_F(FileSelectImageMetadataStripperBrowserTest,
                       RemovesFbmdMetadataDuringUploadAndCleansUpTempFile) {
  // Selecting implictly invokes the stripping event.
  content::WebContents* web_contents =
      OpenTabAndSelectFiles({fbmd_test_image_path_});

  // 1. Check stripping created a temporary file.
  const std::vector<base::FilePath> temp_files = strip_completed_future_.Take();
  ASSERT_EQ(1u, temp_files.size());
  const base::FilePath temp_path = temp_files[0];

  // 2. Check the bytes the server received are the scrubbed bytes.
  EXPECT_FALSE(ContainsFbmd(UploadFileAndInterceptContent(web_contents)))
      << "FBMD metadata should have been stripped before upload";

  // 3. Check Closing the tab tears down the file chooser, which schedules
  // deletion of the temporary files. Nothing should be left behind after the
  // upload flow.
  browser()->tab_strip_model()->CloseWebContentsAt(
      1, TabCloseTypes::CLOSE_USER_GESTURE);
  EXPECT_TRUE(base::test::RunUntil([&]() { return !PathExists(temp_path); }))
      << "Temporary upload file should be deleted after the upload completes";
}

IN_PROC_BROWSER_TEST_F(FileSelectImageMetadataStripperBrowserTest,
                       RepickAfterStripUsesFreshTempFileAndCleansUpAll) {
  // Selecting implictly invokes the stripping event.
  content::WebContents* web_contents =
      OpenTabAndSelectFiles({fbmd_test_image_path_});

  // Keep track of the temporary file which was created for this selection. Will
  // be used later.
  const std::vector<base::FilePath> first_temp_files =
      strip_completed_future_.Take();
  ASSERT_EQ(1u, first_temp_files.size());
  const base::FilePath first_temp = first_temp_files[0];
  EXPECT_TRUE(PathExists(first_temp));

  // Pick again.
  // This resets the callback provided to the
  // SetStripCompletedCallbackForTesting so as to intercept any new temporary
  // files when picking again.
  ReArmOnStripCompleteObserver();
  // Pick the same file.
  SelectFilesInPicker(web_contents, {fbmd_test_image_path_});

  // Temporary file created for this new upload.
  const std::vector<base::FilePath> second_temp_files =
      strip_completed_future_.Take();
  ASSERT_EQ(1u, second_temp_files.size());
  const base::FilePath second_temp = second_temp_files[0];

  // 1. Check each pick creates a distinct temporary copy, and the earlier one
  // is kept alive (by its own FileSelectHelper) until the tab goes away.
  EXPECT_NE(first_temp, second_temp);
  EXPECT_TRUE(PathExists(first_temp));
  EXPECT_TRUE(PathExists(second_temp));

  // The most recently picked file is scrubbed and is what gets uploaded.
  EXPECT_FALSE(ContainsFbmd(UploadFileAndInterceptContent(web_contents)))
      << "FBMD metadata should have been stripped before upload";

  // 2. Check Closing the tab tears down every FileSelectHelper, deleting all
  // the temporary files the picks created.
  browser()->tab_strip_model()->CloseWebContentsAt(
      1, TabCloseTypes::CLOSE_USER_GESTURE);
  EXPECT_TRUE(base::test::RunUntil([&]() {
    return !PathExists(first_temp) && !PathExists(second_temp);
  })) << "All temporary upload files should be deleted after the tab closes";
}

IN_PROC_BROWSER_TEST_F(FileSelectImageMetadataStripperBrowserTest,
                       MultipleFileUploadStripsOnlyStrippableImages) {
  // A non-JPEG companion file that the stripper must leave untouched.
  constexpr std::string_view kTextContents = "plain-text-not-an-image";
  // A dummy directory created for this test which will hold the new txt file.
  base::ScopedTempDir scratch_dir;
  base::FilePath text_path;
  // Create a fake txt file.
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(scratch_dir.CreateUniqueTempDir());
    text_path = scratch_dir.GetPath().AppendASCII("notes.txt");
    ASSERT_TRUE(base::WriteFile(text_path, kTextContents));
  }

  content::WebContents* web_contents =
      OpenTabAndSelectFiles({fbmd_test_image_path_, text_path});

  // 1. Check since only the JPEG is strippable, exactly one temporary copy
  // is created.
  const std::vector<base::FilePath> temp_files = strip_completed_future_.Take();
  ASSERT_EQ(1u, temp_files.size());

  // 2. Check the uploaded payload carries both files: the image scrubbed of
  // FBMD and the text file intact.
  const std::string uploaded_body = UploadFileAndInterceptContent(web_contents);
  EXPECT_FALSE(ContainsFbmd(uploaded_body))
      << "FBMD metadata should have been stripped from the JPEG";
  EXPECT_NE(std::string::npos, uploaded_body.find(kTextContents))
      << "The non-image companion file should be uploaded unchanged";

  // Delete the scratch dir here (blocking is disallowed in ScopedTempDir's
  // destructor, which runs on this UI thread at end of scope).
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(scratch_dir.Delete());
  }
}

IN_PROC_BROWSER_TEST_F(FileSelectImageMetadataStripperBrowserTest,
                       CleanJpegIsUploadedUnchangedAndLeavesNoTempFile) {
  // Minimal valid JPEG: SOI + COM segment (our marker) + EOI, no FBMD record.
  constexpr std::string_view kCleanMarker = "NO_META_MARKER";
  const std::vector<uint8_t> clean_jpeg = {
      0xFF, 0xD8,              // SOI
      0xFF, 0xFE, 0x00, 0x10,  // COM segment, length 16 (2 + 14)
      'N',  'O',  '_',  'M',  'E', 'T', 'A',  '_',
      'M',  'A',  'R',  'K',  'E', 'R', 0xFF, 0xD9,  // EOI
  };
  base::ScopedTempDir scratch_dir;
  base::FilePath clean_jpeg_path;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(scratch_dir.CreateUniqueTempDir());
    clean_jpeg_path = scratch_dir.GetPath().AppendASCII("clean.jpg");
    ASSERT_TRUE(base::WriteFile(clean_jpeg_path, clean_jpeg));
  }

  content::WebContents* web_contents = OpenTabAndSelectFiles({clean_jpeg_path});

  // 1. Check the strip ran but found nothing to remove, so it retained no temp
  // file.
  EXPECT_TRUE(strip_completed_future_.Take().empty());

  // 2. Check the original bytes are uploaded unchanged: our marker survives and
  // no FBMD record was introduced.
  const std::string uploaded_body = UploadFileAndInterceptContent(web_contents);
  EXPECT_NE(std::string::npos, uploaded_body.find(kCleanMarker));
  EXPECT_FALSE(ContainsFbmd(uploaded_body));

  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(scratch_dir.Delete());
  }
}

IN_PROC_BROWSER_TEST_F(FileSelectImageMetadataStripperBrowserTest,
                       CancellingPickerDoesNotStripOrCreateTempFile) {
  content::WebContents* web_contents = OpenTabAndCancelPicker();

  // 1. Check the strip pipeline was never entered, so nothing was selected.
  EXPECT_FALSE(strip_completed_future_.IsReady());
  EXPECT_EQ(0,
            content::EvalJs(web_contents,
                            "document.getElementById('fileinput').files.length")
                .ExtractInt());
}

// Stripping disabled fixture.
class FileSelectImageMetadataStripperFeatureDisabledBrowserTest
    : public FileSelectImageMetadataStripperBase {
 public:
  FileSelectImageMetadataStripperFeatureDisabledBrowserTest()
      : FileSelectImageMetadataStripperBase(
            /*strip_metadata=*/false) {}
};

IN_PROC_BROWSER_TEST_F(
    FileSelectImageMetadataStripperFeatureDisabledBrowserTest,
    DoesNotStripOrCreateTempFileWhenFeatureDisabled) {
  content::WebContents* web_contents =
      OpenTabAndSelectFiles({fbmd_test_image_path_});
  // Stripping code shouldn't have been run.
  EXPECT_FALSE(strip_completed_future_.IsReady());

  EXPECT_TRUE(ContainsFbmd(UploadFileAndInterceptContent(web_contents)))
      << "FBMD metadata should be untouched when the feature is disabled";
}

}  // namespace brave
