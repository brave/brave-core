// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/chart_code_plugin.h"

#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

namespace {

constexpr char kDataKey[] = "data";
constexpr char kXKey[] = "x";
constexpr char kLabelsKey[] = "labels";

constexpr size_t kMaxDataPoints = 200;
constexpr size_t kMaxSeries = 10;

}  // namespace

ChartCodePlugin::ChartCodePlugin() = default;

ChartCodePlugin::~ChartCodePlugin() = default;

bool ChartCodePlugin::IsEnabled() {
  return features::kCodeExecutionToolCharts.Get();
}

std::string_view ChartCodePlugin::Description() const {
  return "Use chartUtil.createLineChart(data, labels, existingChartId). "
         "Returns nothing. data is an array of objects, labels is an optional "
         "map of data keys to display labels, and existingChartId is an "
         "optional string ID of a previously created chart to update. You must "
         "use 'x' as the key for the x-axis. A unique chart ID will be "
         "provided in the console output. "
         "Example: chartUtil.createLineChart([{x: 'Jan', sales: "
         "100, profit: 30}, {x: 'Feb', sales: 150, profit: 45}], {sales: "
         "'Sales ($)', profit: 'Profit ($)'}).";
}

std::string_view ChartCodePlugin::InclusionKeyword() const {
  return "chartUtil";
}

std::string_view ChartCodePlugin::SetupScript() const {
  return R"(
const chartUtil = {
  createLineChart: function(data, labels, existingChartId) {
    const chartData = { data: data };
    if (labels) {
      chartData.labels = labels;
    }
    const artifact = { type: 'line_chart', content: chartData };
    if (existingChartId) {
      artifact.id = existingChartId;
    }
    codeExecArtifacts.push(artifact);
  }
};
)";
}

std::optional<std::string_view> ChartCodePlugin::ArtifactType() const {
  return mojom::kLineChartArtifactType;
}

std::optional<std::string> ChartCodePlugin::ValidateArtifact(
    const base::Value& artifact_value) const {
  const auto* chart_dict = artifact_value.GetIfDict();
  if (!chart_dict) {
    return "Chart must be an object";
  }

  const auto* data = chart_dict->FindList(kDataKey);
  if (!data) {
    return "Chart is missing 'data' array";
  }

  if (data->empty()) {
    return "Chart has empty data array";
  }

  if (data->size() > kMaxDataPoints) {
    return "Chart data array exceeds maximum of 200 entries";
  }

  for (const auto& data_entry : *data) {
    const auto* data_item = data_entry.GetIfDict();
    if (!data_item) {
      return "Chart data entry must be an object";
    }

    if (!data_item->contains(kXKey)) {
      return "Chart data entry is missing required 'x' field";
    }

    if (data_item->size() < 2) {
      return "Chart data entry must have 'x' and at least one other field";
    }

    if (data_item->size() - 1 > kMaxSeries) {
      return "Chart data entry exceeds maximum of 10 series";
    }

    for (const auto [key, value] : *data_item) {
      if (key == kXKey) {
        if (!value.is_string() && !value.is_int() && !value.is_double()) {
          return "Chart data entry 'x' field must be a string or number";
        }
      } else if (!value.is_int() && !value.is_double()) {
        return "Chart data entry values (except 'x') must be numbers";
      }
    }
  }

  const auto* labels = chart_dict->Find(kLabelsKey);
  if (labels) {
    const auto* labels_dict = labels->GetIfDict();
    if (!labels_dict) {
      return "Chart labels must be an object";
    }

    if (labels_dict->size() > kMaxSeries) {
      return "Chart labels exceeds maximum of 10 entries";
    }

    for (const auto [key, value] : *labels_dict) {
      if (!value.is_string()) {
        return "Chart label values must be strings";
      }
    }
  }
  const size_t expected_size = labels ? 2u : 1u;
  if (chart_dict->size() != expected_size) {
    return "Chart may only contain 'data' and optional 'labels' fields";
  }

  return std::nullopt;
}

std::optional<std::string> ChartCodePlugin::GetArtifactCreationMessage(
    std::string_view artifact_id) const {
  return base::StrCat({"Chart created with ID: ", artifact_id});
}

}  // namespace ai_chat
