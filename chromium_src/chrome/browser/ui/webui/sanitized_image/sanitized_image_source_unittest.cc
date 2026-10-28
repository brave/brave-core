/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// TI-040: include the upstream unittest so its cases keep running against the
// chromium_src override (see sanitized_image_source.h in this directory), and
// append only the Brave-specific local-network rejection cases.

#include "base/test/test_future.h"

#include <chrome/browser/ui/webui/sanitized_image/sanitized_image_source_unittest.cc>

// Brave: verifies that image URLs whose literal host is loopback/local (or a
// loopback/local hostname form, including IPv4-mapped IPv6 literals) are
// rejected before any network request is made. The guard lives in the
// SanitizedImageSource subclass declared in sanitized_image_source.h.
TEST_F(SanitizedImageSourceTest, BraveLoopbackUrlRejected) {
  base::test::TestFuture<scoped_refptr<base::RefCountedMemory>> future;
  sanitized_image_source_->StartDataRequest(
      GURL(base::StrCat(
          {chrome::kChromeUIImageURL, "?", "https://127.0.0.1:8377/icon.png"})),
      content::WebContents::Getter(), future.GetCallback());
  EXPECT_TRUE(future.Wait());
  EXPECT_FALSE(future.Get());
  EXPECT_EQ(0, test_url_loader_factory_.NumPending());
}

TEST_F(SanitizedImageSourceTest, BraveMappedIpv6LoopbackUrlRejected) {
  base::test::TestFuture<scoped_refptr<base::RefCountedMemory>> future;
  sanitized_image_source_->StartDataRequest(
      GURL(base::StrCat({chrome::kChromeUIImageURL, "?",
                         "https://[::ffff:127.0.0.1]:8377/icon.png"})),
      content::WebContents::Getter(), future.GetCallback());
  EXPECT_TRUE(future.Wait());
  EXPECT_FALSE(future.Get());
  EXPECT_EQ(0, test_url_loader_factory_.NumPending());
}

TEST_F(SanitizedImageSourceTest, BravePrivateAddressUrlRejected) {
  base::test::TestFuture<scoped_refptr<base::RefCountedMemory>> future;
  sanitized_image_source_->StartDataRequest(
      GURL(base::StrCat(
          {chrome::kChromeUIImageURL, "?", "https://192.168.1.10/icon.png"})),
      content::WebContents::Getter(), future.GetCallback());
  EXPECT_TRUE(future.Wait());
  EXPECT_FALSE(future.Get());
  EXPECT_EQ(0, test_url_loader_factory_.NumPending());
}

TEST_F(SanitizedImageSourceTest, BraveLocalhostHostnameRejected) {
  base::test::TestFuture<scoped_refptr<base::RefCountedMemory>> future;
  sanitized_image_source_->StartDataRequest(
      GURL(base::StrCat(
          {chrome::kChromeUIImageURL, "?", "https://localhost:8377/icon.png"})),
      content::WebContents::Getter(), future.GetCallback());
  EXPECT_TRUE(future.Wait());
  EXPECT_FALSE(future.Get());
  EXPECT_EQ(0, test_url_loader_factory_.NumPending());
}

TEST_F(SanitizedImageSourceTest, BraveDotLocalHostnameRejected) {
  base::test::TestFuture<scoped_refptr<base::RefCountedMemory>> future;
  sanitized_image_source_->StartDataRequest(
      GURL(base::StrCat(
          {chrome::kChromeUIImageURL, "?", "https://menu.local/icon.png"})),
      content::WebContents::Getter(), future.GetCallback());
  EXPECT_TRUE(future.Wait());
  EXPECT_FALSE(future.Get());
  EXPECT_EQ(0, test_url_loader_factory_.NumPending());
}

// Brave: verifies that the factory wrapper applies
// kURLLoadOptionBlockLocalRequest to loads that are allowed to proceed, so
// LocalNetworkAccessChecker enforces it on the resolved endpoint for every
// redirect hop.
TEST_F(SanitizedImageSourceTest, BraveLoaderOptionsApplied) {
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

  // While the request is pending (no response queued yet), verify the factory
  // wrapper applied kURLLoadOptionBlockLocalRequest.
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
  EXPECT_TRUE(future.Wait());
  EXPECT_TRUE(future.Get());
}
