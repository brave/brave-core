/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/mime_util.h"

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(PlaylistMimeUtilUnitTest, GetFileExtensionForMimetype) {
  auto compare_result = [](const auto& mimetype, const auto& extension) {
    EXPECT_STREQ(playlist::mime_util::GetFileExtensionForMimetype(mimetype)
                     .value()
                     .c_str(),
                 extension);
  };

  compare_result("application/ogg", FILE_PATH_LITERAL("ogx"));
  compare_result("application/vnd.apple.mpegurl", FILE_PATH_LITERAL("m3u8"));
  compare_result("application/x-mpegurl", FILE_PATH_LITERAL("m3u8"));
  compare_result("audio/mpegurl", FILE_PATH_LITERAL("m3u8"));
  compare_result("audio/x-mpegurl", FILE_PATH_LITERAL("m3u8"));
  compare_result("audio/aac", FILE_PATH_LITERAL("aac"));
  compare_result("audio/flac", FILE_PATH_LITERAL("flac"));
  compare_result("audio/mp3", FILE_PATH_LITERAL("mp3"));
  compare_result("audio/x-mp3", FILE_PATH_LITERAL("mp3"));
  compare_result("audio/mpeg", FILE_PATH_LITERAL("mp3"));
  compare_result("audio/ogg", FILE_PATH_LITERAL("oga"));
  compare_result("audio/wav", FILE_PATH_LITERAL("wav"));
  compare_result("audio/x-wav", FILE_PATH_LITERAL("wav"));
  compare_result("video/webm", FILE_PATH_LITERAL("webm"));
  compare_result("audio/webm", FILE_PATH_LITERAL("weba"));
  compare_result("audio/x-m4a", FILE_PATH_LITERAL("m4a"));
  compare_result("video/3gpp", FILE_PATH_LITERAL("3gp"));
  compare_result("video/mp2t", FILE_PATH_LITERAL("ts"));
  compare_result("audio/mp4", FILE_PATH_LITERAL("mp4"));
  compare_result("video/mp4", FILE_PATH_LITERAL("mp4"));
  compare_result("video/mpeg", FILE_PATH_LITERAL("mpeg"));
  compare_result("video/ogg", FILE_PATH_LITERAL("ogv"));
  compare_result("video/x-m4v", FILE_PATH_LITERAL("m4v"));

  EXPECT_FALSE(
      playlist::mime_util::GetFileExtensionForMimetype("foo").has_value());
}

TEST(PlaylistMimeUtilUnitTest, GetMimeTypeForFileExtension) {
  auto compare_result = [](const auto& extension, const auto& mimetype) {
    EXPECT_STREQ(playlist::mime_util::GetMimeTypeForFileExtension(extension)
                     .value()
                     .c_str(),
                 mimetype);
  };

  compare_result(FILE_PATH_LITERAL("m3u8"), "application/x-mpegurl");
  compare_result(FILE_PATH_LITERAL("aac"), "audio/aac");
  compare_result(FILE_PATH_LITERAL("flac"), "audio/flac");
  compare_result(FILE_PATH_LITERAL("mp3"), "audio/mp3");
  compare_result(FILE_PATH_LITERAL("mp4"), "video/mp4");
  compare_result(FILE_PATH_LITERAL("oga"), "audio/ogg");
  compare_result(FILE_PATH_LITERAL("wav"), "audio/wav");
  compare_result(FILE_PATH_LITERAL("weba"), "audio/webm");
  compare_result(FILE_PATH_LITERAL("m4a"), "audio/x-m4a");
  compare_result(FILE_PATH_LITERAL("3gp"), "video/3gpp");
  compare_result(FILE_PATH_LITERAL("ts"), "video/mp2t");
  compare_result(FILE_PATH_LITERAL("mpeg"), "video/mpeg");
  compare_result(FILE_PATH_LITERAL("ogv"), "video/ogg");
  compare_result(FILE_PATH_LITERAL("ogx"), "application/ogg");
  compare_result(FILE_PATH_LITERAL("webm"), "video/webm");
  compare_result(FILE_PATH_LITERAL("m4v"), "video/x-m4v");

  EXPECT_FALSE(
      playlist::mime_util::GetMimeTypeForFileExtension(FILE_PATH_LITERAL("foo"))
          .has_value());
}

TEST(PlaylistMimeUtilUnitTest, BothMapsShouldBeInSync) {
  const auto supported_mimetypes = playlist::mime_util::GetSupportedMimetypes();
  ASSERT_FALSE(supported_mimetypes.empty());

  base::flat_map<base::FilePath::StringType, std::vector<std::string>>
      extension_to_mimes;

  for (const auto& mimetype : supported_mimetypes) {
    auto extension = playlist::mime_util::GetFileExtensionForMimetype(mimetype);
    ASSERT_TRUE(extension.has_value());
    extension_to_mimes[extension.value()].push_back(mimetype);
  }

  for (const auto& [extension, mimes] : extension_to_mimes) {
    auto mimetype = playlist::mime_util::GetMimeTypeForFileExtension(extension);
    EXPECT_TRUE(mimetype.has_value());
    EXPECT_THAT(mimes, testing::Contains(mimetype.value()));
  }
}
