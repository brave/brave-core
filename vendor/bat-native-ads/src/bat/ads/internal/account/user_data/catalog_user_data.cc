/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/catalog_user_data.h"

#include <utility>

#include "base/values.h"
#include "bat/ads/internal/ad_server/catalog/catalog_util.h"

namespace ads {
namespace user_data {

namespace {

constexpr char kCatalogKey[] = "catalog";
constexpr char kIdKey[] = "id";

}  // namespace

base::DictionaryValue GetCatalog() {
  base::ListValue list;

  base::DictionaryValue dictionary;
  dictionary.SetStringKey(kIdKey, GetCatalogId());
  list.Append(std::move(dictionary));

  base::DictionaryValue user_data;
  user_data.SetKey(kCatalogKey, std::move(list));

  return user_data;
}

}  // namespace user_data
}  // namespace ads
