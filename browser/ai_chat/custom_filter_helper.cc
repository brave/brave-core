// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/custom_filter_helper.h"

#include <utility>

#include "base/base64.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/ai_chat/core/browser/tools/filter_generation_utils.h"
#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_custom_resource_provider.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

void CreateCustomFilterImpl(PrefService* profile_prefs,
                            const std::string& markdown_response,
                            const std::string& domain,
                            CreateCustomFilterCallback callback) {
  // Parse the AI's markdown response
  auto filter_opt = ParseMarkdownFilterResponse(markdown_response);
  if (!filter_opt.has_value()) {
    VLOG(1) << "Failed to parse filter response";
    std::move(callback).Run(false, "Failed to parse AI response");
    return;
  }

  GeneratedFilter filter = filter_opt.value();

  // Validate the generated code
  if (!ValidateFilterCode(filter.code, filter.type)) {
    VLOG(1) << "Filter validation failed";
    std::move(callback).Run(false, "Generated filter failed safety validation");
    return;
  }

  // Get AdBlock service
  auto* ad_block_service = g_brave_browser_process->ad_block_service();
  if (!ad_block_service) {
    VLOG(1) << "AdBlock service not available";
    std::move(callback).Run(false, "AdBlock service not available");
    return;
  }

  // If it's a scriptlet, create the custom resource
  if (filter.type == mojom::GeneratedFilterType::SCRIPTLET) {
    auto* resource_provider = ad_block_service->custom_resource_provider();
    if (!resource_provider) {
      std::move(callback).Run(false,
                             "Custom resource provider not available");
      return;
    }

    // Generate scriptlet name if not provided
    std::string scriptlet_name;
    if (filter.scriptlet_name.has_value()) {
      scriptlet_name = filter.scriptlet_name.value();
    } else {
      scriptlet_name = GenerateSafeScriptletName(filter.description, domain);
    }

    // Base64 encode the JavaScript code
    std::string encoded_code = base::Base64Encode(filter.code);

    // Create resource dictionary
    base::Value::Dict resource;
    resource.Set("name", scriptlet_name);
    resource.Set("content", encoded_code);
    resource.SetByDottedPath("kind.mime", "application/javascript");

    // Add the resource
    resource_provider->AddResource(
        profile_prefs, base::Value(std::move(resource)),
        base::BindOnce(
            [](CreateCustomFilterCallback callback,
               const std::string& filter_rule,
               brave_shields::AdBlockCustomResourceProvider::ErrorCode error) {
              if (error !=
                  brave_shields::AdBlockCustomResourceProvider::ErrorCode::
                      kOk) {
                std::move(callback).Run(false,
                                       "Failed to create custom scriptlet");
                return;
              }

              // Scriptlet created successfully, now add the filter rule
              auto* ad_block_service =
                  g_brave_browser_process->ad_block_service();
              auto* filters_provider =
                  ad_block_service->custom_filters_provider();
              if (!filters_provider) {
                std::move(callback).Run(
                    false, "Custom filters provider not available");
                return;
              }

              // Add the filter rule
              std::string current_filters = filters_provider->GetCustomFilters();
              std::string updated_filters =
                  current_filters.empty()
                      ? filter_rule
                      : base::StrCat({current_filters, "\n", filter_rule});

              if (filters_provider->UpdateCustomFilters(updated_filters)) {
                std::move(callback).Run(true, "");
              } else {
                std::move(callback).Run(false, "Failed to add filter rule");
              }
            },
            std::move(callback), filter.filter_rule));
  } else {
    // CSS selector - just add the filter rule
    auto* filters_provider = ad_block_service->custom_filters_provider();
    if (!filters_provider) {
      std::move(callback).Run(false, "Custom filters provider not available");
      return;
    }

    // Add the filter rule
    std::string current_filters = filters_provider->GetCustomFilters();
    std::string updated_filters =
        current_filters.empty()
            ? filter.filter_rule
            : base::StrCat({current_filters, "\n", filter.filter_rule});

    if (filters_provider->UpdateCustomFilters(updated_filters)) {
      std::move(callback).Run(true, "");
    } else {
      std::move(callback).Run(false, "Failed to add filter rule");
    }
  }
}

}  // namespace ai_chat
