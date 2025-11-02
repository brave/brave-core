// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/custom_filter_helper.h"

#include "base/base64.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/ai_chat/core/browser/tools/filter_generation_utils.h"
#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_custom_resource_provider.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

namespace {

// Callback for scriptlet creation status
void OnScriptletCreated(
    const std::string& filter_rule,
    CreateCustomFilterCallback final_callback,
    brave_shields::AdBlockCustomResourceProvider::ErrorCode error_code) {
  if (error_code != brave_shields::AdBlockCustomResourceProvider::ErrorCode::kOk) {
    std::string error_message;
    switch (error_code) {
      case brave_shields::AdBlockCustomResourceProvider::ErrorCode::kInvalid:
        error_message = "Invalid scriptlet format";
        break;
      case brave_shields::AdBlockCustomResourceProvider::ErrorCode::kAlreadyExists:
        error_message = "Scriptlet with this name already exists";
        break;
      case brave_shields::AdBlockCustomResourceProvider::ErrorCode::kNotFound:
        error_message = "Scriptlet not found";
        break;
      default:
        error_message = "Unknown error creating scriptlet";
    }
    std::move(final_callback).Run(false, error_message);
    return;
  }

  // Scriptlet created successfully, now add the filter rule
  auto* ad_block_service = g_brave_browser_process->ad_block_service();
  if (!ad_block_service) {
    std::move(final_callback).Run(false, "AdBlock service not available");
    return;
  }

  auto* custom_filters_provider = ad_block_service->custom_filters_provider();
  if (!custom_filters_provider) {
    std::move(final_callback).Run(false, "Custom filters provider not available");
    return;
  }

  // Get existing custom filters
  std::string existing_filters = custom_filters_provider->GetCustomFilters();

  // Append the new filter rule
  std::string updated_filters;
  if (!existing_filters.empty() && !base::EndsWith(existing_filters, "\n")) {
    updated_filters = existing_filters + "\n" + filter_rule;
  } else if (!existing_filters.empty()) {
    updated_filters = existing_filters + filter_rule;
  } else {
    updated_filters = filter_rule;
  }

  // Update the custom filters
  custom_filters_provider->UpdateCustomFilters(updated_filters);

  std::move(final_callback).Run(true, "Custom filter created successfully");
}

}  // namespace

void CreateCustomFilterImpl(PrefService* profile_prefs,
                            const std::string& markdown_response,
                            const std::string& domain,
                            CreateCustomFilterCallback callback) {
  // Parse the markdown response
  auto filter = ParseMarkdownFilterResponse(markdown_response, domain);
  if (!filter.has_value()) {
    std::move(callback).Run(false, "Failed to parse markdown response");
    return;
  }

  if (!filter->scriptlet_name.has_value() || filter->scriptlet_name->empty()) {
    std::move(callback).Run(false, "Missing scriptlet name");
    return;
  }

  if (!filter->filter_rule.has_value() || filter->filter_rule->empty()) {
    std::move(callback).Run(false, "Missing filter rule");
    return;
  }

  if (filter->code.empty()) {
    std::move(callback).Run(false, "Missing JavaScript code");
    return;
  }

  // Get AdBlock service
  auto* ad_block_service = g_brave_browser_process->ad_block_service();
  if (!ad_block_service) {
    std::move(callback).Run(false, "AdBlock service not available");
    return;
  }

  auto* custom_resource_provider = ad_block_service->custom_resource_provider();
  if (!custom_resource_provider) {
    std::move(callback).Run(false, "Custom resource provider not available");
    return;
  }

  // Build the scriptlet resource object
  // Format matches the TypeScript Scriptlet class:
  // { name: string, kind: { mime: string }, content: string (base64) }
  base::Value::Dict scriptlet_dict;
  scriptlet_dict.Set("name", *filter->scriptlet_name);

  base::Value::Dict kind_dict;
  kind_dict.Set("mime", "application/javascript");
  scriptlet_dict.Set("kind", std::move(kind_dict));

  // Base64 encode the JavaScript code
  std::string encoded_content = base::Base64Encode(filter->code);
  scriptlet_dict.Set("content", encoded_content);

  // Wrap the dict in a base::Value
  base::Value scriptlet_value(std::move(scriptlet_dict));

  // Create the scriptlet
  custom_resource_provider->AddResource(
      profile_prefs,
      scriptlet_value,
      base::BindOnce(&OnScriptletCreated, *filter->filter_rule, std::move(callback)));
}

}  // namespace ai_chat
