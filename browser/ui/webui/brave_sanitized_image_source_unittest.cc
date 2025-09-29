// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_sanitized_image_source.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/codec/png_codec.h"
#include "url/url_util.h"

namespace {
constexpr char kValidImage[] = "ad_banner.png";
constexpr char kPaddedImage[] = "padded.png.pad";
constexpr char kInvalidImage[] = "normal.js";

std::string LoadTestFile(const std::string& name) {
  base::FilePath path = base::PathService::CheckedGet(brave::DIR_TEST_DATA);
  path = path.AppendASCII(name);

  base::ScopedAllowBlockingForTesting allow_blocking;
  std::string contents;
  base::ReadFileToString(path, &contents);
  return contents;
}

std::string EncodeQuery(const std::string& query) {
  url::RawCanonOutputT<char> buffer;
  url::EncodeURIComponent(query, &buffer);
  return std::string(buffer.data(), buffer.length());
}

}  // namespace

class BraveSanitizedImageSourceTest : public testing::Test {
 public:
  BraveSanitizedImageSourceTest()
      : source_(
            &profile_,
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)) {
    source_.set_pcdn_domain_for_testing("pcdn.brave.com");
  }
  ~BraveSanitizedImageSourceTest() override = default;

  scoped_refptr<base::RefCountedMemory> Decode(
      const GURL& url,
      const std::string& filename,
      const std::string& target_size = "") {
    std::string data = LoadTestFile(filename);
    GURL image_url = GURL(
        "chrome://brave-image/?url=" + EncodeQuery(url.spec()) +
        (target_size.empty() ? ""
                             : "&target_size=" + EncodeQuery(target_size)));
    base::RepeatingCallback<void(const network::ResourceRequest&)> interceptor =
        base::BindLambdaForTesting(
            [&](const network::ResourceRequest& request) {
              const auto accept_header = request.headers.GetHeader("Accept");
              ASSERT_TRUE(accept_header.has_value());
              EXPECT_NE(accept_header->find("image/webp"), std::string::npos);
            });
    test_url_loader_factory_.SetInterceptor(std::move(interceptor));
    test_url_loader_factory_.AddResponse(url.spec(), std::move(data));

    base::RunLoop run_loop;
    scoped_refptr<base::RefCountedMemory> result;
    source_.StartDataRequest(
        image_url, content::WebContents::Getter(),
        base::BindLambdaForTesting(
            [&](scoped_refptr<base::RefCountedMemory> data) {
              result = std::move(data);
              run_loop.Quit();
            }));
    run_loop.Run();
    return result;
  }

 private:
  content::BrowserTaskEnvironment task_environment_;

  data_decoder::test::InProcessDataDecoder data_decoder_;
  network::TestURLLoaderFactory test_url_loader_factory_;

  TestingProfile profile_;
  BraveSanitizedImageSource source_;
};

TEST_F(BraveSanitizedImageSourceTest, ImageIsDecoded) {
  auto url = GURL("https://example.com/image.png");
  EXPECT_TRUE(Decode(url, kValidImage));
}

TEST_F(BraveSanitizedImageSourceTest, ImageIsDecodedAndResized) {
  // original size is 204x212
  auto url = GURL("https://example.com/image.png");

  struct TestCase {
    std::string size;
    int width;
    int height;
  } kTestCases[] = {
      // height = the target height, width has some room to be cropped
      {"10x20", 19, 20},
      // width = the target width, height has some room to be cropped
      {"40x10", 40, 41},
      // the target size is larger than the original size, so the image is not
      // resized
      {"400x3000", 204, 212},
  };

  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(test_case.size);
    auto result = Decode(url, kValidImage, test_case.size);
    ASSERT_TRUE(result);
    // Check the result is PNG resized to 19x20 (to be cutted to 10x20)
    auto bitmap = gfx::PNGCodec::Decode(base::as_byte_span(*result),
                                        gfx::PNGCodec::FORMAT_BGRA);
    ASSERT_TRUE(bitmap);
    EXPECT_EQ(bitmap->width, test_case.width);
    EXPECT_EQ(bitmap->height, test_case.height);
  }
}

TEST_F(BraveSanitizedImageSourceTest, PaddedImageIsDecoded) {
  auto url = GURL("https://pcdn.brave.com/image.png.pad");
  EXPECT_TRUE(Decode(url, kPaddedImage));
}

TEST_F(BraveSanitizedImageSourceTest, InvalidPaddedImageIsNotDecoded) {
  auto url = GURL("https://example.com/image.png.pad");
  EXPECT_FALSE(Decode(url, kInvalidImage));
}

TEST_F(BraveSanitizedImageSourceTest, PaddedImageWithoutDotPadIsNotDecoded) {
  auto url = GURL("https://pcdn.brave.com/image.png");
  EXPECT_FALSE(Decode(url, kPaddedImage));
}

TEST_F(BraveSanitizedImageSourceTest, PaddedImageOnNonBraveCDNIsNotDecoded) {
  auto url = GURL("https://example.com/image.png.pad");
  EXPECT_FALSE(Decode(url, kPaddedImage));
}

TEST_F(BraveSanitizedImageSourceTest, DotPadOnNonBraveCDNButValidIsDecoded) {
  auto url = GURL("https://example.com/image.png.pad");
  EXPECT_TRUE(Decode(url, kValidImage));
}
