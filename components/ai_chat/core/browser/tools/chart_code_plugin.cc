// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/chart_code_plugin.h"

#include "brave/components/ai_chat/core/common/features.h"

namespace ai_chat {

namespace {

constexpr char kChartType[] = "chart";
constexpr char kDataKey[] = "data";
constexpr char kXKey[] = "x";
constexpr char kLabelsKey[] = "labels";

}  // namespace

ChartCodePlugin::ChartCodePlugin() = default;

ChartCodePlugin::~ChartCodePlugin() = default;

bool ChartCodePlugin::IsEnabled() {
  return features::kCodeExecutionToolCharts.Get();
}

std::string_view ChartCodePlugin::Description() const {
  return "Use chartUtil.createLineChart(data, labels) where data is an array "
         "of objects and labels is an optional map of data keys to display "
         "labels. You must use 'x' as the key for the x-axis. "
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
  createLineChart: function(data, labels) {
    const chartData = { data: data };
    if (labels) {
      chartData.labels = labels;
    }
    codeExecArtifacts.push({ type: 'chart', content: chartData });
  }
};
)";
}

std::optional<std::string> ChartCodePlugin::ValidateArtifact(
    const std::string& type,
    const base::Value& artifact_value) const {
  if (type != kChartType) {
    return std::nullopt;
  }

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

  for (const auto& data_entry : *data) {
    const auto* data_item = data_entry.GetIfDict();
    if (!data_item) {
      return "Chart data entry must be an object";
    }

    if (!data_item->Find(kXKey)) {
      return "Chart data entry is missing required 'x' field";
    }

    if (data_item->size() < 2) {
      return "Chart data entry must have 'x' and at least one other field";
    }
  }

  const auto* labels = chart_dict->Find(kLabelsKey);
  if (labels && !labels->is_dict()) {
    return "Chart labels must be an object";
  }

  return std::nullopt;
}

}  // namespace ai_chat
