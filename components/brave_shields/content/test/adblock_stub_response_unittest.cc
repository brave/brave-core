// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/adblock_stub_response.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(AdBlockStubResponse, ScriptDataURL) {
  std::string data_url =
      "data:application/script,<script>alert('hi');</script>";
  std::string data;
  auto resource_response = network::mojom::URLResponseHead::New();
  network::ResourceRequest request;
  brave_shields::MakeStubResponse(data_url, request, &resource_response, &data);
  ASSERT_EQ(data, "<script>alert('hi');</script>");
  ASSERT_EQ(resource_response->mime_type, "application/script");
}

TEST(AdBlockStubResponse, HTMLDataURL) {
  std::string data_url = "data:text/html,<strong>π</strong>";
  std::string data;
  auto resource_response = network::mojom::URLResponseHead::New();
  network::ResourceRequest request;
  brave_shields::MakeStubResponse(data_url, request, &resource_response, &data);
  ASSERT_EQ(data, "<strong>π</strong>");
  ASSERT_EQ(resource_response->mime_type, "text/html");
}

TEST(AdBlockStubResponse, HTMLDataURLPrioritizedOverRequestInfo) {
  std::string data_url = "data:text/xml,pi";
  std::string data;
  network::ResourceRequest request;
  request.headers.AddHeadersFromString("Accept: image/svg");
  auto resource_response = network::mojom::URLResponseHead::New();
  brave_shields::MakeStubResponse(data_url, request, &resource_response, &data);
  ASSERT_EQ(data, "pi");
  ASSERT_EQ(resource_response->mime_type, "text/xml");
}

TEST(AdBlockStubResponse, AcceptHeaderUsedNoDataURL) {
  std::string data;
  network::ResourceRequest request;
  request.headers.AddHeadersFromString("Accept: text/xml");
  auto resource_response = network::mojom::URLResponseHead::New();
  brave_shields::MakeStubResponse("", request, &resource_response, &data);
  ASSERT_EQ(data, "");
  ASSERT_EQ(resource_response->mime_type, "text/xml");
}

TEST(AdBlockStubResponse, HTMLDataURLNoMimeTypeUsesAcceptHeader) {
  std::string data_url = "data:,<num>pi</num>";
  std::string data;
  network::ResourceRequest request;
  request.headers.AddHeadersFromString("Accept: text/xml");
  auto resource_response = network::mojom::URLResponseHead::New();
  brave_shields::MakeStubResponse(data_url, request, &resource_response, &data);
  ASSERT_EQ(data, "<num>pi</num>");
  ASSERT_EQ(resource_response->mime_type, "text/xml");
}
