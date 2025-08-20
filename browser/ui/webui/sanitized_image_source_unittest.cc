// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/sanitized_image_source.h"

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
}  // namespace

class SanitizedImageSourceTest : public testing::Test {
 public:
  SanitizedImageSourceTest()
      : source_(
            &profile_,
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_),
            std::make_unique<SanitizedImageSource::DataDecoderDelegate>()) {
    source_.set_pcdn_domain_for_testing("pcdn.brave.com");
  }
  ~SanitizedImageSourceTest() override = default;

  scoped_refptr<base::RefCountedMemory> Decode(const GURL& url,
                                               const std::string& filename) {
    std::string data = LoadTestFile(filename);
    GURL image_url = GURL("chrome://image/?" + url.spec());
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
  SanitizedImageSource source_;
};

TEST_F(SanitizedImageSourceTest, ImageIsDecoded) {
  auto url = GURL("https://example.com/image.png");
  EXPECT_TRUE(Decode(url, kValidImage));
}

TEST_F(SanitizedImageSourceTest, PaddedImageIsDecoded) {
  auto url = GURL("https://pcdn.brave.com/image.png.pad");
  EXPECT_TRUE(Decode(url, kPaddedImage));
}

TEST_F(SanitizedImageSourceTest, InvalidPaddedImageIsNotDecoded) {
  auto url = GURL("https://example.com/image.png.pad");
  EXPECT_FALSE(Decode(url, kInvalidImage));
}

TEST_F(SanitizedImageSourceTest, PaddedImageWithoutDotPadIsNotDecoded) {
  auto url = GURL("https://pcdn.brave.com/image.png");
  EXPECT_FALSE(Decode(url, kPaddedImage));
}

TEST_F(SanitizedImageSourceTest, PaddedImageOnNonBraveCDNIsNotDecoded) {
  auto url = GURL("https://example.com/image.png.pad");
  EXPECT_FALSE(Decode(url, kPaddedImage));
}

TEST_F(SanitizedImageSourceTest, DotPadOnNonBraveCDNButValidIsDecoded) {
  auto url = GURL("https://example.com/image.png.pad");
  EXPECT_TRUE(Decode(url, kValidImage));
}
