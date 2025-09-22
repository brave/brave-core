/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_API_REQUEST_HELPER_MOCK_API_REQUEST_HELPER_H_
#define BRAVE_COMPONENTS_API_REQUEST_HELPER_MOCK_API_REQUEST_HELPER_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "testing/gmock/include/gmock/gmock.h"

class GURL;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace api_request_helper {

class MockAPIRequestHelper : public APIRequestHelper {
 public:
  MockAPIRequestHelper(
      net::NetworkTrafficAnnotationTag network_traffic_annotation_tag,
      scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory);

  ~MockAPIRequestHelper() override;

  MOCK_METHOD(Ticket,
              Request,
              (const std::string&,
               const GURL&,
               const std::string&,
               const std::string&,
               ResultCallback,
               (const base::flat_map<std::string, std::string>&),
               const APIRequestOptions&,
               ResponseConversionCallback),
              (override));

  MOCK_METHOD(Ticket,
              RequestSSE,
              (const std::string&,
               const GURL&,
               const std::string&,
               const std::string&,
               DataReceivedCallback,
               ResultCallback,
               (const base::flat_map<std::string, std::string>&),
               const APIRequestOptions&),
              (override));
};

}  // namespace api_request_helper

#endif  // BRAVE_COMPONENTS_API_REQUEST_HELPER_MOCK_API_REQUEST_HELPER_H_
