/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/containers/flat_map.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/media/media.h"
#include "bat/ledger/internal/legacy/static_values.h"
#include "build/build_config.h"

namespace braveledger_media {

Media::Media(ledger::LedgerImpl* ledger)
    : ledger_(ledger),
      media_youtube_(new braveledger_media::YouTube(ledger)),
      media_github_(new braveledger_media::GitHub(ledger)) {}

Media::~Media() = default;

// static
std::string Media::GetLinkType(
    const std::string& url,
    const std::string& first_party_url,
    const std::string& referrer) {
  std::string type = braveledger_media::YouTube::GetLinkType(url);

  if (type.empty()) {
    type = braveledger_media::GitHub::GetLinkType(url);
  }

  return type;
}

void Media::ProcessMedia(const base::flat_map<std::string, std::string>& parts,
                         const std::string& type,
                         ledger::mojom::VisitDataPtr visit_data) {
  if (parts.empty() || !visit_data) {
    return;
  }

  if (type == YOUTUBE_MEDIA_TYPE) {
    media_youtube_->ProcessMedia(parts, *visit_data);
    return;
  }

  if (type == GITHUB_MEDIA_TYPE) {
    media_github_->ProcessMedia(parts, *visit_data);
    return;
  }
}

void Media::GetMediaActivityFromUrl(uint64_t window_id,
                                    ledger::mojom::VisitDataPtr visit_data,
                                    const std::string& type,
                                    const std::string& publisher_blob) {
  if (type == YOUTUBE_MEDIA_TYPE) {
    media_youtube_->ProcessActivityFromUrl(window_id, *visit_data);
  } else if (type == GITHUB_MEDIA_TYPE) {
    media_github_->ProcessActivityFromUrl(window_id, *visit_data);
  } else {
    OnMediaActivityError(std::move(visit_data), type, window_id);
  }
}

void Media::OnMediaActivityError(ledger::mojom::VisitDataPtr visit_data,
                                 const std::string& type,
                                 uint64_t window_id) {
  std::string url;
  std::string name;

  if (type == YOUTUBE_MEDIA_TYPE) {
    url = YOUTUBE_DOMAIN;
    name = YOUTUBE_MEDIA_TYPE;
  }

  if (url.empty()) {
    BLOG(0, "Media activity error");
    return;
  }

  visit_data->domain = url;
  visit_data->url = "https://" + url;
  visit_data->path = "/";
  visit_data->name = name;

  ledger_->publisher()->GetPublisherActivityFromUrl(
      window_id,
      std::move(visit_data),
      "");
}

void Media::SaveMediaInfo(const std::string& type,
                          const base::flat_map<std::string, std::string>& data,
                          ledger::PublisherInfoCallback callback) {
  if (type == GITHUB_MEDIA_TYPE) {
    media_github_->SaveMediaInfo(data, callback);
    return;
  }
}

}  // namespace braveledger_media
