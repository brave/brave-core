/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/download/brave_download_manager_delegate.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "chrome/browser/download/download_core_service.h"
#include "chrome/browser/download/download_core_service_factory.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/download/public/common/download_item.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/download_manager.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/download_test_observer.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

struct ImageTestFile {
  std::string_view file_name;
  std::string_view mime_type;
};

const ImageTestFile kJpegExtAndJpegMimeFile{.file_name = "test.jpeg",
                                            .mime_type = "image/jpeg"};

const ImageTestFile kTxtExtAndPlainMimeFile{.file_name = "test.txt",
                                            .mime_type = "text/plain"};

// .jpeg with a non-image MIME type.
const ImageTestFile kJpegExtAndOctetMimeFile{
    .file_name = "octet_stream.jpeg",
    .mime_type = "application/octet-stream"};

// Non-jpeg extension with an image/jpeg Content-Type.
const ImageTestFile kPngExtAndJpegMimeFile{.file_name = "jpeg_mime.png",
                                           .mime_type = "image/jpeg"};

// "Real" JPEG image in //brave/test/data/image_metadata_stripper that contains
// a Facebook FBMD record inside its IPTC metadata.
const ImageTestFile kRealJpegImageAndJpegMimeFile{
    .file_name = "fbmd_test_image.jpg",
    .mime_type = "image/jpeg"};

std::string UrlPath(const ImageTestFile& file) {
  return base::StrCat({"/", file.file_name});
}

// The ASCII marker that begins the Facebook FBMD IPTC record. The stripper
// zeroes the whole record (including this marker), so its absence in the
// downloaded file signals a successful scrub.
constexpr std::string_view kFbmdMarker = "FBMD";

// Mirrors the ContainsFbmd() helper in jpeg_iptc_metadata_stripper_unittest.cc:
// the FBMD record is detected purely by the presence of its ASCII marker.
bool ContainsFbmd(std::string_view data) {
  return data.find(kFbmdMarker) != std::string_view::npos;
}

// The stripping only reads the file, so an image signature followed by filler
// is enough to stand in for a real image.
constexpr std::string_view kFakeJpegContents = "\xff\xd8\xff\xe0-not-a-jpeg-";
constexpr std::string_view kTextFileContents = "not-an-image";

// Serves the test files in the response.
std::unique_ptr<net::test_server::HttpResponse> HandleDownloadRequest(
    const std::string& fbmd_jpeg_contents,
    const net::test_server::HttpRequest& request) {
  const ImageTestFile* test_file = nullptr;
  std::string_view contents;
  if (request.relative_url == UrlPath(kJpegExtAndJpegMimeFile)) {
    test_file = &kJpegExtAndJpegMimeFile;
    contents = kFakeJpegContents;
  } else if (request.relative_url == UrlPath(kRealJpegImageAndJpegMimeFile)) {
    test_file = &kRealJpegImageAndJpegMimeFile;
    contents = fbmd_jpeg_contents;
  } else if (request.relative_url == UrlPath(kTxtExtAndPlainMimeFile)) {
    test_file = &kTxtExtAndPlainMimeFile;
    contents = kTextFileContents;
  } else if (request.relative_url == UrlPath(kJpegExtAndOctetMimeFile)) {
    test_file = &kJpegExtAndOctetMimeFile;
    contents = kTextFileContents;
  } else if (request.relative_url == UrlPath(kPngExtAndJpegMimeFile)) {
    test_file = &kPngExtAndJpegMimeFile;
    contents = kFakeJpegContents;
  } else {
    return nullptr;
  }

  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->set_content_type(test_file->mime_type);
  response->AddCustomHeader(
      "Content-Disposition",
      base::StrCat({"attachment; filename=\"", test_file->file_name, "\""}));
  response->set_content(std::string(contents));
  return response;
}

// Observes OnImageMetadataStripped() without a production-side test hook.
class TestBraveDownloadManagerDelegate : public BraveDownloadManagerDelegate {
 public:
  explicit TestBraveDownloadManagerDelegate(Profile* profile)
      : BraveDownloadManagerDelegate(profile) {}

  void SetOnImageMetadataStrippedCallback(base::OnceClosure callback) {
    on_image_metadata_stripped_callback_ = std::move(callback);
  }

 protected:
  void OnImageMetadataStripped(uint32_t download_id, bool stripped) override {
    BraveDownloadManagerDelegate::OnImageMetadataStripped(download_id,
                                                          stripped);
    if (on_image_metadata_stripped_callback_) {
      std::move(on_image_metadata_stripped_callback_).Run();
    }
  }

 private:
  base::OnceClosure on_image_metadata_stripped_callback_;
};

}  // namespace

class BraveDownloadManagerDelegateBrowserTestBase : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    ASSERT_TRUE(downloads_directory_.CreateUniqueTempDir());
    Profile* profile = chrome_test_utils::GetProfile(this);
    profile->GetPrefs()->SetBoolean(prefs::kPromptForDownload, false);
    DownloadPrefs::FromDownloadManager(profile->GetDownloadManager())
        ->SetDownloadPath(downloads_directory());

    auto test_delegate =
        std::make_unique<TestBraveDownloadManagerDelegate>(profile);
    test_delegate->GetDownloadIdReceiverCallback().Run(
        download::DownloadItem::kInvalidId + 1);
    DownloadCoreServiceFactory::GetForBrowserContext(profile)
        ->SetDownloadManagerDelegateForTesting(std::move(test_delegate));

    // Preload the real FBMD fixture so the request handler (which runs on the
    // test server's background thread) does not have to touch disk. It is kept
    // around so tests can compare the downloaded file against the original.
    {
      base::ScopedAllowBlockingForTesting allow_blocking;
      const base::FilePath fbmd_path =
          base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
              .AppendASCII("brave/test/data/image_metadata_stripper")
              .AppendASCII(kRealJpegImageAndJpegMimeFile.file_name);
      ASSERT_TRUE(base::ReadFileToString(fbmd_path, &fbmd_jpeg_contents_));
    }
    // Sanity check the fixture actually carries an FBMD record to strip.
    ASSERT_TRUE(ContainsFbmd(fbmd_jpeg_contents_));

    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&HandleDownloadRequest, fbmd_jpeg_contents_));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  const base::FilePath& downloads_directory() const {
    return downloads_directory_.GetPath();
  }

  const std::string& fbmd_jpeg_contents() const { return fbmd_jpeg_contents_; }

  TestBraveDownloadManagerDelegate* GetDownloadManagerDelegate() {
    return static_cast<TestBraveDownloadManagerDelegate*>(
        DownloadCoreServiceFactory::GetForBrowserContext(
            chrome_test_utils::GetProfile(this))
            ->GetDownloadManagerDelegate());
  }

  // Cross-platform stand-in for ui_test_utils::DownloadURL (desktop-only).
  void DownloadURL(const GURL& download_url) {
    content::DownloadManager* const download_manager =
        chrome_test_utils::GetProfile(this)->GetDownloadManager();
    content::DownloadTestObserverTerminal observer(
        download_manager, 1,
        content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_ACCEPT);

    // Attachment downloads may not report a successful document navigation.
    std::ignore = chrome_test_utils::NavigateToURL(
        chrome_test_utils::GetActiveWebContents(this), download_url);
    observer.WaitForFinished();
  }

  void AssertFileWasDownloaded(const ImageTestFile& file) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(
        base::PathExists(downloads_directory().AppendASCII(file.file_name)))
        << "File should always finish downloading";
  }

  void DownloadAndExpectIptcStripping(const ImageTestFile& file,
                                      bool should_strip) {
    base::test::TestFuture<void> iptc_metadata_stripper_future;
    GetDownloadManagerDelegate()->SetOnImageMetadataStrippedCallback(
        iptc_metadata_stripper_future.GetCallback());
    DownloadURL(embedded_test_server()->GetURL(UrlPath(file)));
    if (should_strip) {
      EXPECT_TRUE(iptc_metadata_stripper_future.Wait());
    } else {
      EXPECT_FALSE(iptc_metadata_stripper_future.IsReady());
    }
    AssertFileWasDownloaded(file);
  }

  download::DownloadItem* GetCompletedDownload() {
    content::DownloadManager::DownloadVector downloads;
    chrome_test_utils::GetProfile(this)->GetDownloadManager()->GetAllDownloads(
        &downloads);
    EXPECT_EQ(1u, downloads.size());
    return downloads.empty() ? nullptr : downloads[0];
  }

 protected:
  explicit BraveDownloadManagerDelegateBrowserTestBase(bool strip_metadata) {
    feature_list_.InitWithFeatureState(
        image_metadata_stripper::features::kStripImageMetadataV1,
        strip_metadata);
  }

 private:
  base::ScopedTempDir downloads_directory_;
  std::string fbmd_jpeg_contents_;
  base::test::ScopedFeatureList feature_list_;
};

class BraveDownloadManagerDelegateBrowserTest
    : public BraveDownloadManagerDelegateBrowserTestBase {
 public:
  BraveDownloadManagerDelegateBrowserTest()
      : BraveDownloadManagerDelegateBrowserTestBase(true) {}
};

class BraveDownloadManagerDelegateFeatureDisabledBrowserTest
    : public BraveDownloadManagerDelegateBrowserTestBase {
 public:
  BraveDownloadManagerDelegateFeatureDisabledBrowserTest()
      : BraveDownloadManagerDelegateBrowserTestBase(false) {}
};

// TODO(https://github.com/brave/brave-browser/issues/5238): Enable these tests
// on Android. Android seems to need more plugging through of how download
// needs to be tested. See BrowsingDataRemoverBrowserTestBase::DownloadAnItem
// in the upstream which tests download history via platform browser tests.
#if BUILDFLAG(IS_ANDROID)
#define NON_ANDROID_TEST(test) DISABLED_##test
#else
#define NON_ANDROID_TEST(test) test
#endif

// End-to-end coverage: downloading a real JPEG that carries a Facebook FBMD
// IPTC record results in the record being scrubbed from the file on disk.
IN_PROC_BROWSER_TEST_F(
    BraveDownloadManagerDelegateBrowserTest,
    NON_ANDROID_TEST(RemovesFbmdMetadataFromDownloadedJpegImage)) {
  DownloadAndExpectIptcStripping(kRealJpegImageAndJpegMimeFile,
                                 /*should_strip=*/true);

  base::ScopedAllowBlockingForTesting allow_blocking;
  std::string downloaded_contents;
  ASSERT_TRUE(
      base::ReadFileToString(downloads_directory().AppendASCII(
                                 kRealJpegImageAndJpegMimeFile.file_name),
                             &downloaded_contents));

  // Same before/after invariants as StripsFbmdFromTestImage in
  // jpeg_iptc_metadata_stripper_unittest.cc: the source carried an FBMD record,
  // and the stripper zeroes it in place, so the marker is gone from the saved
  // file without changing its size.
  EXPECT_TRUE(ContainsFbmd(fbmd_jpeg_contents()));
  EXPECT_FALSE(ContainsFbmd(downloaded_contents))
      << "FBMD metadata should have been stripped from the download";
  EXPECT_EQ(downloaded_contents.size(), fbmd_jpeg_contents().size());
}

IN_PROC_BROWSER_TEST_F(
    BraveDownloadManagerDelegateBrowserTest,
    NON_ANDROID_TEST(InvokesStrippingForJpegExtWithNonJpegMime)) {
  DownloadAndExpectIptcStripping(kJpegExtAndOctetMimeFile,
                                 /*should_strip=*/true);

  download::DownloadItem* const item = GetCompletedDownload();
  ASSERT_TRUE(item);
  EXPECT_TRUE(
      item->GetTargetFilePath().MatchesExtension(FILE_PATH_LITERAL(".jpeg")));
}

IN_PROC_BROWSER_TEST_F(
    BraveDownloadManagerDelegateBrowserTest,
    NON_ANDROID_TEST(DoesNotInvokeStrippingForNonJpegExtWithJpegMime)) {
  DownloadAndExpectIptcStripping(kPngExtAndJpegMimeFile,
                                 /*should_strip=*/false);

  download::DownloadItem* const item = GetCompletedDownload();
  ASSERT_TRUE(item);
  EXPECT_TRUE(
      item->GetTargetFilePath().MatchesExtension(FILE_PATH_LITERAL(".png")));
}

IN_PROC_BROWSER_TEST_F(
    BraveDownloadManagerDelegateBrowserTest,
    NON_ANDROID_TEST(DoesNotStripMetadataFromNonImageDownload)) {
  DownloadAndExpectIptcStripping(kTxtExtAndPlainMimeFile,
                                 /*should_strip=*/false);
}

IN_PROC_BROWSER_TEST_F(
    BraveDownloadManagerDelegateFeatureDisabledBrowserTest,
    NON_ANDROID_TEST(DoesNotStripMetadataWhenFeatureDisabled)) {
  DownloadAndExpectIptcStripping(kJpegExtAndJpegMimeFile,
                                 /*should_strip=*/false);
}
