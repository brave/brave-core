// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/star_randomness_test_util.h"

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/p3a/constellation/rs/cxx/src/lib.rs.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

MetricLogType ValidateURLAndGetMetricLogType(const GURL& url,
                                             const char* expected_host) {
  std::string url_prefix = base::StrCat({expected_host, "/instances/"});

  EXPECT_TRUE(url.spec().starts_with(url_prefix));

  std::vector<std::string> path_segments = base::SplitString(
      url.path(), "/", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  EXPECT_EQ(path_segments.size(), 4U);

  std::optional<MetricLogType> log_type =
      StringToMetricLogType(path_segments[2]);
  EXPECT_TRUE(log_type.has_value());

  return *log_type;
}

std::string HandleRandomnessRequest(const network::ResourceRequest& request,
                                    uint8_t expected_epoch) {
  EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
  std::string_view request_string(request.request_body->elements()
                                      ->at(0)
                                      .As<network::DataElementBytes>()
                                      .AsStringPiece());

  base::Value::Dict req_parsed_val = base::test::ParseJsonDict(request_string);

  EXPECT_EQ(*req_parsed_val.FindInt("epoch"), expected_epoch);

  rust::Vec<constellation::VecU8> req_points_rust;
  const base::Value::List* points_list = req_parsed_val.FindList("points");

  EXPECT_GE(points_list->size(), 7U);
  EXPECT_LE(points_list->size(), 9U);

  std::transform(
      points_list->begin(), points_list->end(),
      std::back_inserter(req_points_rust), [](const base::Value& val) {
        constellation::VecU8 point_dec_rust;
        std::vector<uint8_t> point_dec = *base::Base64Decode(val.GetString());
        std::copy(point_dec.cbegin(), point_dec.cend(),
                  std::back_inserter(point_dec_rust.data));
        return point_dec_rust;
      });

  auto rand_result =
      constellation::generate_local_randomness(req_points_rust, expected_epoch);

  EXPECT_GE(rand_result.points.size(), 7U);
  EXPECT_LE(rand_result.points.size(), 9U);

  base::Value::List resp_points_list;
  for (const constellation::VecU8& resp_point_rust : rand_result.points) {
    std::vector<uint8_t> resp_point;
    std::copy(resp_point_rust.data.cbegin(), resp_point_rust.data.cend(),
              std::back_inserter(resp_point));
    resp_points_list.Append(base::Base64Encode(resp_point));
  }

  base::Value::Dict resp_value;
  resp_value.Set("epoch", base::Value(expected_epoch));
  resp_value.Set("points", std::move(resp_points_list));
  std::string resp_json;
  base::JSONWriter::Write(resp_value, &resp_json);
  return resp_json;
}

std::string HandleInfoRequest(const network::ResourceRequest& request,
                              MetricLogType log_type,
                              uint8_t current_epoch,
                              const char* next_epoch_time) {
  EXPECT_EQ(request.method, net::HttpRequestHeaders::kGetMethod);

  return base::StrCat(
      {"{\"currentEpoch\":", base::NumberToString(current_epoch),
       ", \"nextEpochTime\": \"", next_epoch_time, "\"}"});
}

}  // namespace p3a
