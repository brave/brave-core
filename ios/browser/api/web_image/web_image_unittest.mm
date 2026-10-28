/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#import "brave/ios/browser/api/web_image/web_image+private.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

WebImageDownloader* MakeDownloader(network::TestURLLoaderFactory* factory) {
  return [[WebImageDownloader alloc]
      initWithURLLoaderFactory:base::MakeRefCounted<
                                   network::WeakWrapperSharedURLLoaderFactory>(
                                   factory)];
}

}  // namespace

class WebImageDownloaderTest : public PlatformTest {};

// Brave: verifies that image URLs with loopback/local literal hosts (or
// localhost/*.local hostnames) are rejected before any network request is
// made, mirroring the desktop and iOS image data source protection.
TEST_F(WebImageDownloaderTest, LocalNetworkUrlsNotFetched) {
  base::test::TaskEnvironment task_environment;

  NSArray<NSString*>* local_urls = @[
    @"https://127.0.0.1:8377/icon.png",
    @"https://[::ffff:127.0.0.1]:8377/icon.png",
    @"https://192.168.1.10/icon.png",
    @"https://localhost:8377/icon.png",
    @"https://menu.local/icon.png",
  ];

  for (NSString* url_string in local_urls) {
    network::TestURLLoaderFactory test_factory;
    WebImageDownloader* downloader = MakeDownloader(&test_factory);

    base::RunLoop run_loop;
    __block BOOL called = NO;
    __block UIImage* image = nil;
    [downloader downloadImage:[NSURL URLWithString:url_string]
                   completion:^(UIImage* result, NSInteger http_response_code,
                                NSURL* response_url) {
                     called = YES;
                     image = result;
                     run_loop.Quit();
                   }];
    run_loop.Run();

    EXPECT_TRUE(called) << url_string.UTF8String;
    EXPECT_EQ(nil, image) << url_string.UTF8String;
    EXPECT_EQ(0, test_factory.NumPending()) << url_string.UTF8String;
  }
}

// Control: a public URL must still be fetched (the guard only rejects local
// network destinations).
TEST_F(WebImageDownloaderTest, PublicUrlIsFetched) {
  base::test::TaskEnvironment task_environment;
  network::TestURLLoaderFactory test_factory;
  test_factory.AddResponse("https://example.com/icon.png", "not-an-image");
  WebImageDownloader* downloader = MakeDownloader(&test_factory);

  base::RunLoop run_loop;
  __block BOOL called = NO;
  [downloader
      downloadImage:[NSURL URLWithString:@"https://example.com/icon.png"]
         completion:^(UIImage* result, NSInteger http_response_code,
                      NSURL* response_url) {
           called = YES;
           run_loop.Quit();
         }];
  run_loop.Run();

  EXPECT_TRUE(called);
  EXPECT_EQ(0, test_factory.NumPending());
}
