/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/text_embedding_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_embedding/text_embedding_feature.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/embedding_processing_ref_counted_proxy.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/text_embedding_resource_constants.h"
#include "brave/components/brave_ads/core/internal/resources/language_components.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  return UserHasOptedInToBravePrivateAds();
}

}  // namespace

TextEmbeddingResource::TextEmbeddingResource() {
  AdsClientHelper::AddObserver(this);
}

TextEmbeddingResource::~TextEmbeddingResource() {
  AdsClientHelper::RemoveObserver(this);
}

void TextEmbeddingResource::EmbedText(const std::string& text,
                                      EmbedTextCallback callback) const {
  if (!DidLoad() || !embedding_processing_) {
    return;
  }

  embedding_processing_->Get()
      .AsyncCall(&EmbeddingProcessingRefCountedProxy::EmbedText)
      .WithArgs(text)
      .Then(std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void TextEmbeddingResource::MaybeLoad() {
  if (manifest_version_ && DoesRequireResource()) {
    Load();
  }
}

void TextEmbeddingResource::MaybeLoadOrReset() {
  DidLoad() ? MaybeReset() : MaybeLoad();
}

void TextEmbeddingResource::Load() {
  did_load_ = true;

  AdsClientHelper::GetInstance()->LoadFileResource(
      kTextEmbeddingResourceId, kTextEmbeddingResourceVersion.Get(),
      base::BindOnce(&TextEmbeddingResource::OnLoadFileResource,
                     weak_factory_.GetWeakPtr()));
}

void TextEmbeddingResource::OnLoadFileResource(base::File file) {
  if (!file.IsValid() || !manifest_version_) {
    return;
  }

  embedding_processing_.emplace();
  embedding_processing_->Get()
      .AsyncCall(&EmbeddingProcessingRefCountedProxy::Load)
      .WithArgs(std::move(file), *manifest_version_)
      .Then(base::BindOnce(&TextEmbeddingResource::LoadCallback,
                           weak_factory_.GetWeakPtr()));
}

void TextEmbeddingResource::LoadCallback(
    base::expected<bool, std::string> result) {
  if (!result.has_value()) {
    BLOG(0, "Failed to initialize " << kTextEmbeddingResourceId
                                    << " text embedding resource ("
                                    << result.error() << ")");
    return embedding_processing_.reset();
  }

  if (!result.value()) {
    BLOG(1, kTextEmbeddingResourceId
                << " text embedding resource is not available");
    return embedding_processing_.reset();
  }

  BLOG(1, "Successfully loaded " << kTextEmbeddingResourceId
                                 << " text embedding resource");

  BLOG(1, "Successfully initialized " << kTextEmbeddingResourceId
                                      << " text embedding resource version "
                                      << kTextEmbeddingResourceVersion.Get());
}

void TextEmbeddingResource::MaybeReset() {
  if (DidLoad() && !DoesRequireResource()) {
    Reset();
  }
}

void TextEmbeddingResource::Reset() {
  BLOG(1, "Reset " << kTextEmbeddingResourceId << " text embedding resource");
  embedding_processing_.reset();
  did_load_ = false;
}

void TextEmbeddingResource::OnNotifyLocaleDidChange(
    const std::string& /*locale*/) {
  MaybeLoad();
}

void TextEmbeddingResource::OnNotifyPrefDidChange(const std::string& path) {
  if (path == prefs::kEnabled) {
    MaybeLoadOrReset();
  }
}

void TextEmbeddingResource::OnNotifyDidUpdateResourceComponent(
    const std::string& manifest_version,
    const std::string& id) {
  if (!IsValidLanguageComponentId(id)) {
    return;
  }

  if (manifest_version == manifest_version_) {
    return;
  }

  manifest_version_ = manifest_version;

  MaybeLoad();
}

}  // namespace brave_ads
