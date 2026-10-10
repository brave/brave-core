/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_dash_parser.h"

#include <string>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace playlist {

namespace {

constexpr char kBaseUrl[] = "https://example.com/dash/manifest.mpd";

}  // namespace

class PlaylistDashParserTest : public testing::Test {
 protected:
  base::expected<DashManifest, DashParseError> Parse(const std::string& xml) {
    base::test::TestFuture<base::expected<DashManifest, DashParseError>> future;
    ParseDashManifest(xml, GURL(kBaseUrl), future.GetCallback());
    return future.Take();
  }

  base::test::TaskEnvironment task_environment_;
  data_decoder::test::InProcessDataDecoder data_decoder_;
};

TEST_F(PlaylistDashParserTest, SegmentTemplateWithTimeline) {
  auto parsed = Parse(R"(<?xml version="1.0"?>
      <MPD mediaPresentationDuration="PT2.7S">
        <Period>
          <AdaptationSet contentType="video">
            <Representation id="0" codecs="avc1.4d401f" bandwidth="500000"
                            width="1280" height="720">
              <SegmentTemplate timescale="30000"
                  initialization="init_$RepresentationID$.m4s"
                  media="chunk_$RepresentationID$_$Number%05d$.m4s"
                  startNumber="1">
                <SegmentTimeline>
                  <S t="0" d="30000" r="1" />
                  <S d="15000" />
                </SegmentTimeline>
              </SegmentTemplate>
            </Representation>
          </AdaptationSet>
        </Period>
      </MPD>)");

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->video);
  EXPECT_FALSE(parsed->audio);

  const auto& video = *parsed->video;
  EXPECT_EQ(GURL("https://example.com/dash/init_0.m4s"), video.initialization);
  // `r="1"` means the entry repeats once, so three segments in total.
  ASSERT_EQ(3u, video.segments.size());
  EXPECT_EQ(GURL("https://example.com/dash/chunk_0_00001.m4s"),
            video.segments[0]);
  EXPECT_EQ(GURL("https://example.com/dash/chunk_0_00002.m4s"),
            video.segments[1]);
  EXPECT_EQ(GURL("https://example.com/dash/chunk_0_00003.m4s"),
            video.segments[2]);
  EXPECT_EQ(base::Seconds(1), video.durations[0]);
  EXPECT_EQ(base::Seconds(0.5), video.durations[2]);
  EXPECT_EQ(1280, *video.width);
  EXPECT_EQ("avc1.4d401f", video.codecs);
}

TEST_F(PlaylistDashParserTest, SeparatesAudioAndVideo) {
  auto parsed = Parse(R"(<?xml version="1.0"?>
      <MPD>
        <Period duration="PT2S">
          <AdaptationSet contentType="video">
            <Representation id="v" bandwidth="500000">
              <SegmentTemplate timescale="1" duration="1"
                  initialization="vi.m4s" media="v$Number$.m4s" />
            </Representation>
          </AdaptationSet>
          <AdaptationSet contentType="audio">
            <Representation id="a" bandwidth="64000">
              <SegmentTemplate timescale="1" duration="1"
                  initialization="ai.m4s" media="a$Number$.m4s" />
            </Representation>
          </AdaptationSet>
        </Period>
      </MPD>)");

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->video);
  ASSERT_TRUE(parsed->audio);
  // No timeline, so the count comes from the period duration.
  EXPECT_EQ(2u, parsed->video->segments.size());
  EXPECT_EQ(GURL("https://example.com/dash/v1.m4s"),
            parsed->video->segments[0]);
  EXPECT_EQ(GURL("https://example.com/dash/a2.m4s"),
            parsed->audio->segments[1]);
}

TEST_F(PlaylistDashParserTest, PicksHighestBandwidthRepresentation) {
  auto parsed = Parse(R"(<?xml version="1.0"?>
      <MPD>
        <Period duration="PT1S">
          <AdaptationSet contentType="video">
            <Representation id="low" bandwidth="1000">
              <SegmentTemplate timescale="1" duration="1" media="low$Number$.m4s" />
            </Representation>
            <Representation id="high" bandwidth="900000">
              <SegmentTemplate timescale="1" duration="1" media="high$Number$.m4s" />
            </Representation>
          </AdaptationSet>
        </Period>
      </MPD>)");

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->video);
  EXPECT_EQ(GURL("https://example.com/dash/high1.m4s"),
            parsed->video->segments[0]);
  EXPECT_EQ(900000u, parsed->video->bandwidth);
}

TEST_F(PlaylistDashParserTest, ResolvesBaseUrl) {
  auto parsed = Parse(R"(<?xml version="1.0"?>
      <MPD>
        <BaseURL>https://cdn.example.com/v1/</BaseURL>
        <Period duration="PT1S">
          <AdaptationSet contentType="video">
            <Representation id="v" bandwidth="1">
              <SegmentTemplate timescale="1" duration="1" media="seg$Number$.m4s" />
            </Representation>
          </AdaptationSet>
        </Period>
      </MPD>)");

  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(GURL("https://cdn.example.com/v1/seg1.m4s"),
            parsed->video->segments[0]);
}

TEST_F(PlaylistDashParserTest, SegmentList) {
  auto parsed = Parse(R"(<?xml version="1.0"?>
      <MPD>
        <Period>
          <AdaptationSet contentType="audio">
            <Representation id="a" bandwidth="64000">
              <SegmentList timescale="1" duration="2">
                <Initialization sourceURL="init.mp4" />
                <SegmentURL media="one.m4s" />
                <SegmentURL media="two.m4s" />
              </SegmentList>
            </Representation>
          </AdaptationSet>
        </Period>
      </MPD>)");

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->audio);
  EXPECT_EQ(GURL("https://example.com/dash/init.mp4"),
            parsed->audio->initialization);
  EXPECT_EQ(2u, parsed->audio->segments.size());
  EXPECT_EQ(base::Seconds(2), parsed->audio->durations[1]);
}

TEST_F(PlaylistDashParserTest, RejectsProtectedContent) {
  auto parsed = Parse(R"(<?xml version="1.0"?>
      <MPD>
        <Period duration="PT1S">
          <AdaptationSet contentType="video">
            <ContentProtection schemeIdUri="urn:uuid:EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED" />
            <Representation id="v" bandwidth="1">
              <SegmentTemplate timescale="1" duration="1" media="s$Number$.m4s" />
            </Representation>
          </AdaptationSet>
        </Period>
      </MPD>)");

  ASSERT_FALSE(parsed.has_value());
  EXPECT_EQ(DashParseError::kEncrypted, parsed.error());
}

TEST_F(PlaylistDashParserTest, RejectsByteRangeAddressing) {
  // `SegmentBase` makes every segment a byte range of one remote file, which
  // isn't something we can store as separate files.
  auto parsed = Parse(R"(<?xml version="1.0"?>
      <MPD>
        <Period duration="PT1S">
          <AdaptationSet contentType="video">
            <Representation id="v" bandwidth="1">
              <BaseURL>whole.mp4</BaseURL>
              <SegmentBase indexRange="0-500" />
            </Representation>
          </AdaptationSet>
        </Period>
      </MPD>)");

  ASSERT_FALSE(parsed.has_value());
  EXPECT_EQ(DashParseError::kUnsupportedAddressing, parsed.error());
}

TEST_F(PlaylistDashParserTest, RejectsUnboundedRepeat) {
  // `r="-1"` means "until the period ends", which a live manifest uses and we
  // can't resolve into a fixed list.
  auto parsed = Parse(R"(<?xml version="1.0"?>
      <MPD>
        <Period>
          <AdaptationSet contentType="video">
            <Representation id="v" bandwidth="1">
              <SegmentTemplate timescale="1" media="s$Number$.m4s">
                <SegmentTimeline><S t="0" d="1" r="-1" /></SegmentTimeline>
              </SegmentTemplate>
            </Representation>
          </AdaptationSet>
        </Period>
      </MPD>)");

  ASSERT_FALSE(parsed.has_value());
  EXPECT_EQ(DashParseError::kUnsupportedAddressing, parsed.error());
}

TEST_F(PlaylistDashParserTest, RejectsGarbage) {
  EXPECT_EQ(DashParseError::kMalformed, Parse("not xml at all").error());
  EXPECT_EQ(DashParseError::kMalformed,
            Parse("<?xml version=\"1.0\"?><NotAnMPD/>").error());
}

TEST_F(PlaylistDashParserTest, IgnoresSubtitleAdaptationSets) {
  auto parsed = Parse(R"(<?xml version="1.0"?>
      <MPD>
        <Period duration="PT1S">
          <AdaptationSet contentType="text">
            <Representation id="t" bandwidth="100">
              <SegmentTemplate timescale="1" duration="1" media="t$Number$.vtt" />
            </Representation>
          </AdaptationSet>
          <AdaptationSet contentType="video">
            <Representation id="v" bandwidth="1">
              <SegmentTemplate timescale="1" duration="1" media="v$Number$.m4s" />
            </Representation>
          </AdaptationSet>
        </Period>
      </MPD>)");

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->video);
  EXPECT_FALSE(parsed->audio);
}

}  // namespace playlist
