// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/chart_code_plugin.h"

#include "brave/components/ai_chat/core/common/features.h"

namespace ai_chat {

namespace {

constexpr char kChartsKey[] = "charts";
constexpr char kDataKey[] = "data";
constexpr char kNameKey[] = "name";
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
         "labels. Example: chartUtil.createLineChart([{month: 'Jan', sales: "
         "100, profit: 30}, {month: 'Feb', sales: 150, profit: 45}], {sales: "
         "'Sales ($)', profit: 'Profit ($)'}).";
}

std::string_view ChartCodePlugin::InclusionKeyword() const {
  return "chartUtil";
}

std::string_view ChartCodePlugin::SetupScript() const {
  return R"(
const chartUtil = {
  createLineChart: function(data, labels) {
    if (!codeExecOutput.charts) {
      codeExecOutput.charts = [];
    }
    const chartData = { data: data };
    if (labels) {
      chartData.labels = labels;
    }
    codeExecOutput.charts.push(chartData);
  }
};
)";
}

std::optional<std::string> ChartCodePlugin::ValidateOutput(
    const base::Value::Dict& output) const {
  const auto* charts = output.FindList(kChartsKey);
  if (!charts) {
    return std::nullopt;
  }

  for (const auto& chart : *charts) {
    const auto* chart_dict = chart.GetIfDict();
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

      if (!data_item->Find(kNameKey)) {
        return "Chart data entry is missing required 'name' field";
      }

      if (data_item->size() < 2) {
        return "Chart data entry must have 'name' and at least one other field";
      }
    }

    const auto* labels = chart_dict->Find(kLabelsKey);
    if (labels && !labels->is_dict()) {
      return "Chart labels must be an object";
    }
  }

  return std::nullopt;
}

}  // namespace ai_chat
