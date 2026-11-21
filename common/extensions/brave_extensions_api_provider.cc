/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/extensions/brave_extensions_api_provider.h"

#include <string_view>

#include "brave/common/extensions/api/api_features.h"
#include "brave/common/extensions/api/behavior_features.h"
#include "brave/common/extensions/api/generated_schemas.h"
#include "brave/common/extensions/api/grit/brave_api_resources.h"
#include "brave/common/extensions/api/manifest_features.h"
#include "extensions/common/features/json_feature_provider_source.h"
#include "extensions/common/permissions/permissions_info.h"

namespace extensions {

BraveExtensionsAPIProvider::BraveExtensionsAPIProvider() = default;
BraveExtensionsAPIProvider::~BraveExtensionsAPIProvider() = default;

StaticFeatureTable BraveExtensionsAPIProvider::GetAPIFeatures() const {
  return GetBraveAPIFeatures();
}

StaticFeatureTable BraveExtensionsAPIProvider::GetManifestFeatures() const {
  return GetBraveManifestFeatures();
}

StaticFeatureTable BraveExtensionsAPIProvider::GetPermissionFeatures() const {
  return {};
}

StaticFeatureTable BraveExtensionsAPIProvider::GetBehaviorFeatures() const {
  // No brave-specific behavior features.
  return {};
}

void BraveExtensionsAPIProvider::AddAPIJSONSources(
    JSONFeatureProviderSource* json_source) {
  json_source->LoadJSON(IDR_BRAVE_EXTENSION_API_FEATURES);
}

bool BraveExtensionsAPIProvider::IsAPISchemaGenerated(const std::string& name) {
  return api::BraveGeneratedSchemas::IsGenerated(name);
}

std::string_view BraveExtensionsAPIProvider::GetAPISchema(
    const std::string& name) {
  return api::BraveGeneratedSchemas::Get(name);
}

void BraveExtensionsAPIProvider::RegisterPermissions(
    PermissionsInfo* permissions_info) {
  // No brave-specific permissions.
}

void BraveExtensionsAPIProvider::RegisterManifestHandlers(
    ManifestHandlerRegistry* registry) {
  // No brave-specific manifest handlers.
}

}  // namespace extensions
