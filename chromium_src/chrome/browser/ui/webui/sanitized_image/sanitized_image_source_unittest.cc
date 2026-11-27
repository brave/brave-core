/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/test_future.h"

#include <chrome/browser/ui/webui/sanitized_image/sanitized_image_source_unittest.cc>

namespace {

class BraveSanitizedImageSourceNetworkTest : public SanitizedImageSourceTest {
 protected:
  void ExpectRejected(const std::string& image_url_spec) {
    base::test::TestFuture<scoped_refptr<base::RefCountedMemory>> future;
    sanitized_image_source_->StartDataRequest(
        GURL(base::StrCat({chrome::kChromeUIImageURL, "?", image_url_spec})),
        content::WebContents::Getter(), future.GetCallback());
    ASSERT_TRUE(future.Wait());
    EXPECT_FALSE(future.Get());
    EXPECT_EQ(0, test_url_loader_factory_.NumPending());
  }
};

}  // namespace

TEST_F(BraveSanitizedImageSourceNetworkTest,
       BraveLocalAndLoopbackUrlsRejected) {
  const char* kRejectedUrls[] = {
      "https://127.0.0.1:8377/icon.png",
      "https://[::ffff:127.0.0.1]:8377/icon.png",
      "https://192.168.1.10/icon.png",
      "https://localhost:8377/icon.png",
      "https://menu.local/icon.png",
  };
  for (const auto* image_url : kRejectedUrls) {
    SCOPED_TRACE(image_url);
    ExpectRejected(image_url);
  }
}

TEST_F(BraveSanitizedImageSourceNetworkTest, BraveLoaderOptionsApplied) {
  constexpr char kImageUrl[] = "https://foo.com/img.png";
  EXPECT_CALL(*mock_data_decoder_delegate_, DecodeImage(testing::_, testing::_))
      .WillOnce([](const std::string&,
                   SanitizedImageSource::DecodeImageCallback callback) {
        std::move(callback).Run(MakeImageFrame(SK_ColorRED)->bitmap);
      });

  base::test::TestFuture<scoped_refptr<base::RefCountedMemory>> future;
  sanitized_image_source_->StartDataRequest(
      GURL(base::StrCat({chrome::kChromeUIImageURL, "?", kImageUrl})),
      content::WebContents::Getter(), future.GetCallback());

  test_url_loader_factory_.WaitForRequest(GURL(kImageUrl));
  bool block_local_requested = false;
  for (const auto& pending_request :
       *test_url_loader_factory_.pending_requests()) {
    if (pending_request.request.url.spec() == kImageUrl) {
      block_local_requested =
          (pending_request.options &
           network::mojom::kURLLoadOptionBlockLocalRequest) != 0;
    }
  }
  EXPECT_TRUE(block_local_requested);

  test_url_loader_factory_.AddResponse(kImageUrl, "abc");
  ASSERT_TRUE(future.Wait());
  EXPECT_TRUE(future.Get());
}
