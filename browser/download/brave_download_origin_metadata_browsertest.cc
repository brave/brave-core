/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/download_test_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_MAC)
#include <sys/xattr.h>
#endif

namespace {

constexpr char kPageHost[] = "page.test";
constexpr char kDownloadHost[] = "download.test";
constexpr char kDownloadFileName[] = "file.txt";

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  if (request.relative_url == "/page.html") {
    response->set_content_type("text/html");
  } else if (request.relative_url == "/file.txt") {
    response->set_content_type("text/plain");
    response->AddCustomHeader("Content-Disposition", "attachment");
    response->set_content("contents");
  } else {
    return nullptr;
  }
  return response;
}

// Where-from metadata on the file, as text: kMDItemWhereFroms on macOS, the
// mark of the web on Windows.
std::string GetOriginMetadata(const base::FilePath& file) {
  std::string metadata;

#if BUILDFLAG(IS_MAC)
  constexpr char kWhereFroms[] = "com.apple.metadata:kMDItemWhereFroms";
  const ssize_t size =
      getxattr(file.value().c_str(), kWhereFroms, nullptr, 0, 0, 0);
  if (size > 0) {
    std::string where_froms(size, '\0');
    getxattr(file.value().c_str(), kWhereFroms, where_froms.data(), size, 0, 0);
    metadata += where_froms;
  }
#elif BUILDFLAG(IS_WIN)
  base::ReadFileToString(
      base::FilePath(file.value() + FILE_PATH_LITERAL(":Zone.Identifier")),
      &metadata);
#endif  // BUILDFLAG(IS_MAC)

  return metadata;
}

}  // namespace

class BraveDownloadOriginMetadataBrowserTest : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    ASSERT_TRUE(downloads_directory_.CreateUniqueTempDir());
    Profile* profile = chrome_test_utils::GetProfile(this);
    profile->GetPrefs()->SetBoolean(prefs::kPromptForDownload, false);
    DownloadPrefs::FromDownloadManager(profile->GetDownloadManager())
        ->SetDownloadPath(downloads_directory_.GetPath());

    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&HandleRequest));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 protected:
  base::ScopedTempDir downloads_directory_;
};

// Downloads must not record where they came from: not the download URL, the
// referrer, or the page that started the download.
IN_PROC_BROWSER_TEST_F(BraveDownloadOriginMetadataBrowserTest,
                       DownloadsDoNotRecordWhereTheyCameFrom) {
  content::WebContents* web_contents =
      chrome_test_utils::GetActiveWebContents(this);
  ASSERT_TRUE(content::NavigateToURL(
      web_contents, embedded_test_server()->GetURL(kPageHost, "/page.html")));

  const GURL download_url =
      embedded_test_server()->GetURL(kDownloadHost, "/file.txt");
  content::DownloadTestObserverTerminal observer(
      chrome_test_utils::GetProfile(this)->GetDownloadManager(), 1,
      content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_ACCEPT);
  // Started by the page, like a user clicking a download link.
  ASSERT_TRUE(content::ExecJs(
      web_contents, content::JsReplace("location.href = $1", download_url)));
  observer.WaitForFinished();

  base::ScopedAllowBlockingForTesting allow_blocking;
  const base::FilePath file =
      downloads_directory_.GetPath().AppendASCII(kDownloadFileName);
  ASSERT_TRUE(base::PathExists(file));

  const std::string metadata = GetOriginMetadata(file);
#if BUILDFLAG(IS_WIN)
  // Fail closed if the Zone.Identifier is missing, since the host checks would
  // have nothing to inspect.
  ASSERT_NE(std::string::npos, metadata.find("ZoneId=")) << metadata;
#endif
  EXPECT_EQ(std::string::npos, metadata.find(kPageHost)) << metadata;
  EXPECT_EQ(std::string::npos, metadata.find(kDownloadHost)) << metadata;
}
