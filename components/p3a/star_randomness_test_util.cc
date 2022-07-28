// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/star_randomness_test_util.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/time/time_to_iso8601.h"
#include "brave/components/nested_star/src/lib.rs.h"
#include "net/http/http_request_headers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

std::string HandleRandomnessRequest(const network::ResourceRequest& request,
                                    uint8_t expected_epoch) {
  EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
  base::StringPiece request_string(request.request_body->elements()
                                       ->at(0)
                                       .As<network::DataElementBytes>()
                                       .AsStringPiece());

  base::Value req_parsed_val = *base::JSONReader::Read(request_string);

  EXPECT_EQ(*req_parsed_val.FindIntKey("epoch"), expected_epoch);

  rust::Vec<nested_star::VecU8> req_points_rust;
  base::Value::List& points_list =
      req_parsed_val.FindListKey("points")->GetList();

  EXPECT_NE(points_list.size(), 0U);

  std::transform(
      points_list.cbegin(), points_list.cend(),
      std::back_inserter(req_points_rust), [](const base::Value& val) {
        nested_star::VecU8 point_dec_rust;
        std::vector<uint8_t> point_dec = *base::Base64Decode(val.GetString());
        std::copy(point_dec.cbegin(), point_dec.cend(),
                  std::back_inserter(point_dec_rust.data));
        return point_dec_rust;
      });

  auto rand_result =
      nested_star::generate_local_randomness(req_points_rust, expected_epoch);

  EXPECT_NE(rand_result.points.size(), 0U);

  base::Value resp_points_value(base::Value::Type::LIST);
  for (const nested_star::VecU8& resp_point_rust : rand_result.points) {
    std::vector<uint8_t> resp_point;
    std::copy(resp_point_rust.data.cbegin(), resp_point_rust.data.cend(),
              std::back_inserter(resp_point));
    resp_points_value.Append(base::Base64Encode(resp_point));
  }

  base::Value resp_value(base::Value::Type::DICT);
  resp_value.SetKey("epoch", base::Value(expected_epoch));
  resp_value.SetKey("points", std::move(resp_points_value));
  std::string resp_json;
  base::JSONWriter::Write(resp_value, &resp_json);
  return resp_json;
}

}  // namespace brave
