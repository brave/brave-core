/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/thread_test_helper.h"
#include "brave/common/brave_paths.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "extensions/test/result_catcher.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace extensions {
namespace {

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  if (request.relative_url == "/valid_thumbnail" ||
      request.relative_url == "/valid_video_file_1" ||
      request.relative_url == "/valid_audio_file_1") {
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("image/gif");
    http_response->set_content("thumbnail");
  } else {
    http_response->set_code(net::HTTP_NOT_FOUND);
  }

  return std::move(http_response);
}

}  // namespace

class BravePlaylistsApiTest : public ExtensionApiTest {
 public:
  void SetUp() override {
    base::ScopedAllowBlockingForTesting allow_blocking;
    brave::RegisterPathProvider();
    base::PathService::Get(brave::DIR_TEST_DATA, &extension_dir_);
    extension_dir_ = extension_dir_.AppendASCII("extensions/api_test");
    ExtensionApiTest::SetUp();
  }

  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    // Set up embedded test server to handle fake responses.
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(base::BindRepeating(&HandleRequest));
    ASSERT_TRUE(https_server_->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void runTest(std::string js) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ResultCatcher catcher;
    const Extension* extension =
        LoadExtension(extension_dir_.AppendASCII("bravePlaylists"));
    ASSERT_TRUE(extension);
    ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
        browser()->profile(), brave_extension_id, js));
    ASSERT_TRUE(catcher.GetNextResult()) << message_;
  }

  std::string valid_thumbnail_url() {
    return https_server_->GetURL("example.com", "/valid_thumbnail").spec();
  }

  std::string invalid_thumbnail_url() {
    return https_server_->GetURL("example.com", "/invalid_thumbnail").spec();
  }

  std::string valid_video_url() {
    return https_server_->GetURL("example.com", "/valid_video_file_1").spec();
  }

  std::string invalid_video_url() {
    return https_server_->GetURL("example.com", "/invalid_video_file_1").spec();
  }

  std::string valid_audio_url() {
    return https_server_->GetURL("example.com", "/valid_audio_file_1").spec();
  }

  base::FilePath extension_dir_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(BravePlaylistsApiTest, HasAccess) {
  runTest("testHasAccess()");
}

IN_PROC_BROWSER_TEST_F(BravePlaylistsApiTest, Initialize) {
  runTest("testInitialize()");
}

IN_PROC_BROWSER_TEST_F(BravePlaylistsApiTest, CreatePlaylistNoCrash) {
  runTest("testCreatePlaylistNoCrash()");
}

IN_PROC_BROWSER_TEST_F(BravePlaylistsApiTest, CreatePlaylistValid) {
  runTest(
      "testCreatePlaylist('" + valid_thumbnail_url() + "', '" +
      valid_video_url() +
      "', '', ['added','thumbnail_ready','play_ready_partial','play_ready'])");
}

IN_PROC_BROWSER_TEST_F(BravePlaylistsApiTest,
                       CreatePlaylistValidSeparateAudio) {
  runTest("testCreatePlaylist('" + valid_thumbnail_url() + "', '" +
          valid_video_url() + "', '" + valid_audio_url() +
          "', ['added','thumbnail_ready','play_ready_partial','play_ready'])");
}

IN_PROC_BROWSER_TEST_F(BravePlaylistsApiTest, ThumbnailFailedButVideoOK) {
  runTest(
      "testCreatePlaylist('" + invalid_thumbnail_url() + "', '" +
      valid_video_url() +
      "', '', ['added','thumbnail_failed','play_ready_partial','play_ready'])");
}

IN_PROC_BROWSER_TEST_F(BravePlaylistsApiTest, ThumbnailOKButVideoFailed) {
  runTest("testCreatePlaylist('" + valid_thumbnail_url() + "', '" +
          invalid_video_url() +
          "', '', ['added','thumbnail_ready','play_ready_partial','aborted'])");
}

IN_PROC_BROWSER_TEST_F(BravePlaylistsApiTest, ThumbnailFailedAndVideoFailed) {
  runTest(
      "testCreatePlaylist('" + invalid_thumbnail_url() + "', '" +
      invalid_video_url() +
      "', '', ['added','thumbnail_failed','play_ready_partial','aborted'])");
}

}  // namespace
}  // namespace extensions
