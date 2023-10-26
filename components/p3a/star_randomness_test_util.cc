// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/star_randomness_test_util.h"

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_writer.h"
#include "base/test/values_test_util.h"
#include "brave/components/p3a/constellation/rs/cxx/src/lib.rs.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

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

  EXPECT_EQ(points_list->size(), 8U);

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

  EXPECT_EQ(rand_result.points.size(), 8U);

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

}  // namespace p3a
