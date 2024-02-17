/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/json/values_util.h"
#include "base/ranges/algorithm.h"
#include "base/values.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "brave/components/playlist/common/playlist_render_frame_observer_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace playlist {
namespace {

auto RequiredProperties() {
  auto item = mojom::PlaylistItem::New();
  item->name = "Video 1";
  item->page_source = GURL("https://example.com");
  item->page_redirected = GURL("https://example.com/redirected");
  item->media_source = GURL("https://example.com/video.mp4");
  item->media_path = GURL("https://example.com/video.mp4");

  std::vector<mojom::PlaylistItemPtr> items;
  items.push_back(std::move(item));

  return std::pair(base::Value::List().Append(
                       base::Value::Dict()
                           .Set("name", "Video 1")
                           .Set("pageTitle", "Example page")
                           .Set("pageSrc", "https://example.com/redirected")
                           .Set("mimeType", "video")
                           .Set("src", "https://example.com/video.mp4")
                           .Set("srcIsMediaSourceObjectURL", false)),
                   std::move(items));
}

auto RequiredPropertiesMissing() {
  return std::pair(base::Value::List().Append(base::Value::Dict()),
                   std::vector<mojom::PlaylistItemPtr>());
}

auto OptionalProperties() {
  auto item = mojom::PlaylistItem::New();
  item->name = "Video 1";
  item->page_source = GURL("https://example.com");
  item->page_redirected = GURL("https://example.com/redirected");
  item->media_source = GURL("https://example.com/video.mp4");
  item->media_path = GURL("https://example.com/video.mp4");
  item->author = "Me";
  item->thumbnail_source = GURL("https://example.com/thumbnail.jpg");
  item->thumbnail_path = GURL("https://example.com/thumbnail.jpg");
  item->duration = base::TimeDeltaToValue(base::Seconds(1234)).GetString();

  std::vector<mojom::PlaylistItemPtr> items;
  items.push_back(std::move(item));

  return std::pair(
      base::Value::List().Append(
          base::Value::Dict()
              .Set("name", "Video 1")
              .Set("pageTitle", "Example page")
              .Set("pageSrc", "https://example.com/redirected")
              .Set("mimeType", "video")
              .Set("src", "https://example.com/video.mp4")
              .Set("srcIsMediaSourceObjectURL", false)
              .Set("author", "Me")
              .Set("thumbnail", "https://example.com/thumbnail.jpg")
              .Set("duration", 1234.0)),
      std::move(items));
}

auto UnsupportedSrcSchemeHTTP() {
  return std::pair(base::Value::List().Append(
                       base::Value::Dict()
                           .Set("name", "Video 1")
                           .Set("pageTitle", "Example page")
                           .Set("pageSrc", "https://example.com/redirected")
                           .Set("mimeType", "video")
                           .Set("src", "http://example.com/video.mp4")
                           .Set("srcIsMediaSourceObjectURL", false)),
                   std::vector<mojom::PlaylistItemPtr>());
}

auto UnsupportedSrcSchemeBlobHTTP() {
  return std::pair(base::Value::List().Append(
                       base::Value::Dict()
                           .Set("name", "Video 1")
                           .Set("pageTitle", "Example page")
                           .Set("pageSrc", "https://example.com/redirected")
                           .Set("mimeType", "video")
                           .Set("src", "blob:http://example.com/12345")
                           .Set("srcIsMediaSourceObjectURL", false)),
                   std::vector<mojom::PlaylistItemPtr>());
}
}  // namespace

using ParamType = std::pair<
    std::string,  // test name suffix
    std::pair<base::Value::List, std::vector<mojom::PlaylistItemPtr>> (*)()>;

class PlaylistRenderFrameObserverHelperTest
    : public testing::TestWithParam<ParamType> {};

TEST_P(PlaylistRenderFrameObserverHelperTest,
       ExtractPlaylistItemsInTheBackground) {
  auto [list, expected_items] = std::get<1>(GetParam())();
  auto items =
      ExtractPlaylistItems(GURL("https://example.com"), std::move(list));
  base::ranges::for_each(
      items,
      [](std::string& id) {
        EXPECT_FALSE(id.empty());
        id = "";  // so that we can use operator==
      },
      &mojom::PlaylistItem::id);
  EXPECT_EQ(items, expected_items);
}

INSTANTIATE_TEST_SUITE_P(
    Playlist,
    PlaylistRenderFrameObserverHelperTest,
    testing::Values(
        ParamType("RequiredProperties", &RequiredProperties),
        ParamType("RequiredPropertiesMissing", &RequiredPropertiesMissing),
        ParamType("OptionalProperties", &OptionalProperties),
        ParamType("UnsupportedSrcSchemeHTTP", &UnsupportedSrcSchemeHTTP),
        ParamType("UnsupportedSrcSchemeBlobHTTP",
                  &UnsupportedSrcSchemeBlobHTTP)),
    [](const auto& info) { return std::get<0>(info.param); });

}  // namespace playlist
