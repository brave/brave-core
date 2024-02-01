/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/car_content_requester.h"
#include <string>

#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

constexpr char kGatewayUrlFormatParamName[] = "format";
constexpr char kGatewayUrlFormatParamVal[] = "car";

constexpr char kGatewayUrlDagScopeParamName[] = "dag-scope";
constexpr char kGatewayUrlDagScopeParamVal[] = "entity";

GURL SetUrlParams(const GURL& url) {
  GURL::Replacements replacements;
  auto query_str = base::JoinString(
      {base::StringPrintf("%s=%s", kGatewayUrlFormatParamName,
                          kGatewayUrlFormatParamVal)
           .c_str(),
       base::StringPrintf("%s=%s", kGatewayUrlDagScopeParamName,
                          kGatewayUrlDagScopeParamVal)
           .c_str()},
      "&");
  replacements.SetQueryStr(query_str);
  return url.ReplaceComponents(replacements);
}

}  // namespace

namespace ipfs::ipld {

CarContentRequester::CarContentRequester(
    const GURL& url,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs)
    : ContentRequester(url, url_loader_factory, prefs) {}

CarContentRequester::~CarContentRequester() = default;

const GURL CarContentRequester::GetGatewayRequestUrl() const {
  return SetUrlParams(ContentRequester::GetGatewayRequestUrl());
}

}  // namespace ipfs::ipld
