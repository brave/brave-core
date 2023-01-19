/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/catalog_user_data.h"

#include <utility>

#include "bat/ads/internal/catalog/catalog_util.h"

namespace ads::user_data {

namespace {

constexpr char kCatalogKey[] = "catalog";
constexpr char kIdKey[] = "id";

}  // namespace

base::Value::Dict GetCatalog() {
  base::Value::List list;

  base::Value::Dict dict;
  dict.Set(kIdKey, GetCatalogId());
  list.Append(std::move(dict));

  base::Value::Dict user_data;
  user_data.Set(kCatalogKey, std::move(list));

  return user_data;
}

}  // namespace ads::user_data
