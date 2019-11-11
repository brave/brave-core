/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/adblock_stub_response.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_response.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(AdBlockStubResponse, ScriptRedirectDataURL) {
  std::string redirect =
    "data:application/script,<script>alert('hi');</script>";
  std::string data;
  network::ResourceRequest request;
  network::ResourceResponseHead resource_response;
  brave_shields::MakeStubResponse(redirect, request, &resource_response, &data);
  ASSERT_STREQ(data.c_str(), "<script>alert('hi');</script>");
  ASSERT_STREQ(resource_response.mime_type.c_str(), "application/script");
}

TEST(AdBlockStubResponse, HTMLRedirectDataURL) {
  std::string redirect = "data:text/html,<strong>π</strong>";
  std::string data;
  network::ResourceRequest request;
  network::ResourceResponseHead resource_response;
  brave_shields::MakeStubResponse(redirect, request, &resource_response, &data);
  ASSERT_STREQ(data.c_str(), "<strong>π</strong>");
  ASSERT_STREQ(resource_response.mime_type.c_str(), "text/html");
}
