/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/autofill/ui/autofill_image_fetcher_impl.h"

#include "base/containers/span.h"
#include "components/autofill/core/browser/payments/constants.h"
#include "components/image_fetcher/core/mock_image_fetcher.h"
#include "components/image_fetcher/core/request_metadata.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace autofill {

class AutofillImageFetcherImplForTest : public AutofillImageFetcherImpl {
 public:
  AutofillImageFetcherImplForTest()
      : AutofillImageFetcherImpl(nullptr),
        mock_image_fetcher_(
            std::make_unique<image_fetcher::MockImageFetcher>()) {}
  ~AutofillImageFetcherImplForTest() override = default;

  image_fetcher::MockImageFetcher* mock_image_fetcher() const {
    return mock_image_fetcher_.get();
  }

  void SimulateOnCardArtImageFetched(const GURL& url, const gfx::Image& image) {
    OnCardArtImageFetched(url, image, image_fetcher::RequestMetadata());
  }

  void SimulateOnValuableImageFetched(const GURL& url,
                                      const gfx::Image& image) {
    OnValuableImageFetched(url, image, image_fetcher::RequestMetadata());
  }

  // AutofillImageFetcher:
  image_fetcher::ImageFetcher* GetImageFetcher() override {
    return mock_image_fetcher_.get();
  }

 private:
  std::unique_ptr<image_fetcher::MockImageFetcher> mock_image_fetcher_;
};

class AutofillImageFetcherImplTest : public testing::Test {
 public:
  AutofillImageFetcherImplTest()
      : autofill_image_fetcher_impl_(
            std::make_unique<AutofillImageFetcherImplForTest>()) {}

  image_fetcher::MockImageFetcher* mock_image_fetcher() {
    return autofill_image_fetcher_impl_->mock_image_fetcher();
  }

  AutofillImageFetcherImplForTest* autofill_image_fetcher() {
    return autofill_image_fetcher_impl_.get();
  }

 private:
  std::unique_ptr<AutofillImageFetcherImplForTest> autofill_image_fetcher_impl_;
};

TEST_F(AutofillImageFetcherImplTest, FetchCreditCardArtImage) {
  GURL fake_url1 = GURL("https://www.example.com/fake_image1");
  GURL fake_url2 = GURL(kCapitalOneCardArtUrl);

  // Our stub is empty. Expect mock_image_fetcher not to be called.
  EXPECT_CALL(*mock_image_fetcher(), FetchImageAndData_).Times(0);
  autofill_image_fetcher()->FetchCreditCardArtImagesForURLs(
      {fake_url1, fake_url2},
      base::span_from_ref(AutofillImageFetcherBase::ImageSize::kSmall));

  // OnCardArtImageFetched stub is empty so no images should be cached.
  gfx::Image fake_image = gfx::test::CreateImage(4, 4);
  autofill_image_fetcher()->SimulateOnCardArtImageFetched(fake_url1,
                                                          fake_image);
  autofill_image_fetcher()->SimulateOnCardArtImageFetched(fake_url2,
                                                          fake_image);
  EXPECT_EQ(
      nullptr,
      autofill_image_fetcher()->GetCachedImageForUrl(
          fake_url1, AutofillImageFetcherBase::ImageType::kCreditCardArtImage));
  EXPECT_EQ(
      nullptr,
      autofill_image_fetcher()->GetCachedImageForUrl(
          fake_url2, AutofillImageFetcherBase::ImageType::kCreditCardArtImage));
}

TEST_F(AutofillImageFetcherImplTest, FetchValuableImage) {
  gfx::Image fake_image = gfx::test::CreateImage(4, 4);
  GURL fake_url = GURL("https://www.example.com/fake_image");

  // Our stub is empty. Expect mock_image_fetcher not to be called.
  EXPECT_CALL(*mock_image_fetcher(), FetchImageAndData_).Times(0);
  autofill_image_fetcher()->FetchValuableImagesForURLs({fake_url});

  // OnValuableImageFetched stub is empty so no images should be cached.
  autofill_image_fetcher()->SimulateOnValuableImageFetched(fake_url,
                                                           fake_image);
  EXPECT_EQ(nullptr,
            autofill_image_fetcher()->GetCachedImageForUrl(
                fake_url, AutofillImageFetcherBase::ImageType::kValuableImage));
}

}  // namespace autofill
