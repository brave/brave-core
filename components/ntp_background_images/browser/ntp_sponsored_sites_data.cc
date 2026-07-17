/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_sites_data.h"

#include <optional>
#include <utility>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"

namespace ntp_background_images {

namespace {

constexpr int kExpectedSchemaVersion = 1;
constexpr int kExpectedTileVersion = 1;

constexpr char kTilesKey[] = "tiles";
constexpr char kTileVersionKey[] = "version";
constexpr char kTileTitleKey[] = "title";
constexpr char kTileAdDisclosureKey[] = "adDisclosure";
constexpr char kTileTargetUrlKey[] = "targetUrl";
constexpr char kTileImageKey[] = "image";
constexpr char kTileImageRelativeUrlKey[] = "relativeUrl";

std::optional<NTPSponsoredSite> ParseSite(const base::DictValue& dict,
                                          const base::FilePath& installed_dir,
                                          const std::string& url_prefix) {
  std::optional<int> tile_version = dict.FindInt(kTileVersionKey);
  if (!tile_version || *tile_version != kExpectedTileVersion) {
    return std::nullopt;
  }

  const std::string* const title = dict.FindString(kTileTitleKey);
  if (!title || title->empty()) {
    return std::nullopt;
  }

  const std::string* const target_url = dict.FindString(kTileTargetUrlKey);
  if (!target_url || target_url->empty()) {
    return std::nullopt;
  }

  const base::DictValue* const image_dict = dict.FindDict(kTileImageKey);
  if (!image_dict) {
    return std::nullopt;
  }

  const std::string* const relative_url =
      image_dict->FindString(kTileImageRelativeUrlKey);
  if (!relative_url || relative_url->empty()) {
    return std::nullopt;
  }

  const base::FilePath image_file_path =
      installed_dir.Append(base::FilePath::FromUTF8Unsafe(*relative_url));
  if (!base::PathExists(image_file_path)) {
    DVLOG(1) << "Sponsored site image not found: " << image_file_path;
    return std::nullopt;
  }

  const std::string* const ad_disclosure =
      dict.FindString(kTileAdDisclosureKey);

  NTPSponsoredSite sponsored_site;
  sponsored_site.relative_image_url_spec = url_prefix + *relative_url;
  sponsored_site.title = *title;
  sponsored_site.ad_disclosure = ad_disclosure ? *ad_disclosure : "";
  sponsored_site.target_url = GURL(*target_url);

  if (!sponsored_site.target_url.is_valid() ||
      !sponsored_site.target_url.SchemeIsHTTPOrHTTPS()) {
    return std::nullopt;
  }

  return sponsored_site;
}

}  // namespace

NTPSponsoredSitesData::NTPSponsoredSitesData() = default;
NTPSponsoredSitesData::NTPSponsoredSitesData(
    const base::DictValue& dict,
    const base::FilePath& installed_dir,
    const std::string& url_prefix)
    : NTPSponsoredSitesData() {
  std::optional<int> schema_version_value = dict.FindInt(kSchemaVersionKey);
  if (schema_version_value != kExpectedSchemaVersion) {
    DVLOG(1) << "Unsupported tiles.json schema version: "
             << schema_version_value.value_or(-1);
    return;
  }
  schema_version = *schema_version_value;

  const base::ListValue* const sponsored_sites = dict.FindList(kTilesKey);
  if (!sponsored_sites) {
    return;
  }

  for (const auto& value : *sponsored_sites) {
    const base::DictValue* const tile_dict = value.GetIfDict();
    if (!tile_dict) {
      continue;
    }
    std::optional<NTPSponsoredSite> sponsored_site =
        ParseSite(*tile_dict, installed_dir, url_prefix);
    if (sponsored_site) {
      sites.push_back(std::move(*sponsored_site));
    }
  }
}

NTPSponsoredSitesData::NTPSponsoredSitesData(
    const NTPSponsoredSitesData& data) = default;

NTPSponsoredSitesData& NTPSponsoredSitesData::operator=(
    const NTPSponsoredSitesData& data) = default;

NTPSponsoredSitesData::NTPSponsoredSitesData(
    NTPSponsoredSitesData&& other) noexcept = default;

NTPSponsoredSitesData& NTPSponsoredSitesData::operator=(
    NTPSponsoredSitesData&& other) noexcept = default;

NTPSponsoredSitesData::~NTPSponsoredSitesData() = default;

bool NTPSponsoredSitesData::IsValid() const {
  return !sites.empty();
}

}  // namespace ntp_background_images
