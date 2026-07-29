/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/download/brave_download_manager_delegate.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kPngImageFileName[] = "test.png";
constexpr char kJpegImageFileName[] = "test.jpeg";
constexpr char kTextFileName[] = "test.txt";

// The stripping only reads the file, so an image signature followed by filler
// is enough to stand in for a real image.
constexpr std::string_view kFakePngContents = "\x89PNG\r\n\x1a\n-not-a-png-";
constexpr std::string_view kFakeJpegContents = "\xff\xd8\xff\xe0-not-a-jpeg-";
constexpr std::string_view kTextFileContents = "not-an-image";

std::unique_ptr<net::test_server::HttpResponse> HandleDownloadRequest(
    const net::test_server::HttpRequest& request) {
  std::string mime_type;
  std::string_view contents;
  if (request.relative_url == std::string("/") + kPngImageFileName) {
    mime_type = "image/png";
    contents = kFakePngContents;
  } else if (request.relative_url == std::string("/") + kJpegImageFileName) {
    mime_type = "image/jpeg";
    contents = kFakeJpegContents;
  } else if (request.relative_url == std::string("/") + kTextFileName) {
    mime_type = "text/plain";
    contents = kTextFileContents;
  } else {
    return nullptr;
  }

  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->set_content_type(mime_type);
  response->AddCustomHeader(
      "Content-Disposition",
      "attachment; filename=\"" + request.relative_url.substr(1) + "\"");
  response->set_content(std::string(contents));
  return response;
}

// Installs a test only callback which is fired once the iptc stripping of a
// download item has finished, and clears it again on destruction.
class ScopedImageMetadataStrippedCallbackForTesting {
 public:
  explicit ScopedImageMetadataStrippedCallbackForTesting(
      base::OnceClosure callback)
      : callback_(std::move(callback)) {
    BraveDownloadManagerDelegate::SetOnImageMetadataStrippedCallbackForTesting(
        &callback_);
  }

  ~ScopedImageMetadataStrippedCallbackForTesting() {
    BraveDownloadManagerDelegate::SetOnImageMetadataStrippedCallbackForTesting(
        nullptr);
  }

  ScopedImageMetadataStrippedCallbackForTesting(
      const ScopedImageMetadataStrippedCallbackForTesting&) = delete;
  ScopedImageMetadataStrippedCallbackForTesting& operator=(
      const ScopedImageMetadataStrippedCallbackForTesting&) = delete;

 private:
  base::OnceClosure callback_;
};

}  // namespace

class BraveDownloadManagerDelegateBrowserTestBase
    : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    ASSERT_TRUE(downloads_directory_.CreateUniqueTempDir());
    browser()->profile()->GetPrefs()->SetBoolean(prefs::kPromptForDownload,
                                                 false);
    DownloadPrefs::FromDownloadManager(
        browser()->profile()->GetDownloadManager())
        ->SetDownloadPath(downloads_directory());

    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&HandleDownloadRequest));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  const base::FilePath& downloads_directory() const {
    return downloads_directory_.GetPath();
  }

  void AssertFileWasDownloaded(std::string_view file_name) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::PathExists(downloads_directory().AppendASCII(file_name)))
        << "File should always finish downloading";
  }

 protected:
  explicit BraveDownloadManagerDelegateBrowserTestBase(bool strip_metadata) {
    feature_list_.InitWithFeatureState(
        image_metadata_stripper::features::kStripDownloadedImageMetadata,
        strip_metadata);
  }

 private:
  base::ScopedTempDir downloads_directory_;
  base::test::ScopedFeatureList feature_list_;
};

// TODO(https://github.com/brave/brave-browser/issues/5238): Add tests to see we
// actually scrub the FBMD metadata once the logic is in-place.
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

IN_PROC_BROWSER_TEST_F(BraveDownloadManagerDelegateBrowserTest,
                       StripsMetadataFromDownloadedPngImage) {
  base::test::TestFuture<void> iptc_metadata_stripper_future;
  ScopedImageMetadataStrippedCallbackForTesting scoped_callback(
      iptc_metadata_stripper_future.GetCallback());

  ui_test_utils::DownloadURL(
      browser(),
      embedded_test_server()->GetURL(std::string("/") + kPngImageFileName));

  // RemoveIptcMetadata() ran for the download, and the download still made it
  // to disk afterwards.
  EXPECT_TRUE(iptc_metadata_stripper_future.Wait());
  // Check the download was completed.
  AssertFileWasDownloaded(kPngImageFileName);
}

IN_PROC_BROWSER_TEST_F(BraveDownloadManagerDelegateBrowserTest,
                       StripsMetadataFromDownloadedJpegImage) {
  base::test::TestFuture<void> iptc_metadata_stripper_future;
  ScopedImageMetadataStrippedCallbackForTesting scoped_callback(
      iptc_metadata_stripper_future.GetCallback());

  ui_test_utils::DownloadURL(
      browser(),
      embedded_test_server()->GetURL(std::string("/") + kJpegImageFileName));

  // RemoveIptcMetadata() ran for the download, and the download still made it
  // to disk afterwards.
  EXPECT_TRUE(iptc_metadata_stripper_future.Wait());
  // Check the download was completed.
  AssertFileWasDownloaded(kJpegImageFileName);
}

IN_PROC_BROWSER_TEST_F(BraveDownloadManagerDelegateBrowserTest,
                       DoesNotStripMetadataFromNonImageDownload) {
  base::test::TestFuture<void> iptc_metadata_stripper_future;
  ScopedImageMetadataStrippedCallbackForTesting scoped_callback(
      iptc_metadata_stripper_future.GetCallback());

  ui_test_utils::DownloadURL(browser(), embedded_test_server()->GetURL(
                                            std::string("/") + kTextFileName));

  // Ensure the callback inside the iptc_metadata_stripper_future was never
  // fired as the file's MIME type didn't correspond to image.
  EXPECT_FALSE(iptc_metadata_stripper_future.IsReady());

  // Ensure the file was still downloaded regardless.
  AssertFileWasDownloaded(kTextFileName);
}

IN_PROC_BROWSER_TEST_F(BraveDownloadManagerDelegateFeatureDisabledBrowserTest,
                       DoesNotStripMetadataWhenFeatureDisabled) {
  base::test::TestFuture<void> iptc_metadata_stripper_future;
  ScopedImageMetadataStrippedCallbackForTesting scoped_callback(
      iptc_metadata_stripper_future.GetCallback());

  ui_test_utils::DownloadURL(
      browser(),
      embedded_test_server()->GetURL(std::string("/") + kPngImageFileName));

  EXPECT_FALSE(iptc_metadata_stripper_future.IsReady());
  AssertFileWasDownloaded(kPngImageFileName);
}
