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
#include "brave/common/extensions/api/permission_features.h"
#include "extensions/common/features/json_feature_provider_source.h"
#include "extensions/common/permissions/permissions_info.h"

namespace extensions {

BraveExtensionsAPIProvider::BraveExtensionsAPIProvider() = default;
BraveExtensionsAPIProvider::~BraveExtensionsAPIProvider() = default;

void BraveExtensionsAPIProvider::AddAPIFeatures(FeatureProvider* provider) {
  AddBraveAPIFeatures(provider);
}

void BraveExtensionsAPIProvider::AddManifestFeatures(
    FeatureProvider* provider) {
  AddBraveManifestFeatures(provider);
}

void BraveExtensionsAPIProvider::AddPermissionFeatures(
    FeatureProvider* provider) {
  AddBravePermissionFeatures(provider);
}

void BraveExtensionsAPIProvider::AddBehaviorFeatures(
    FeatureProvider* provider) {
  // No brave-specific behavior features.
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

void BraveExtensionsAPIProvider::RegisterManifestHandlers() {
  // No brave-specific manifest handlers.
}

}  // namespace extensions
