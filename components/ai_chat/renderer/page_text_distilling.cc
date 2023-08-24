// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/renderer/page_text_distilling.h"

#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_tree.h"
#include "ui/accessibility/platform/inspect/ax_tree_formatter.h"
#include "content/public/browser/ax_inspect_factory.h"
#include "ui/accessibility/ax_tree_manager.h"
#include "ui/accessibility/platform/inspect/ax_inspect.h"
#include "ui/accessibility/platform/ax_platform_node.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "base/command_line.h"

using ui::AXPropertyFilter;

namespace ai_chat {

namespace {

static const ax::mojom::Role kContentRoles[] = {ax::mojom::Role::kHeading,
                                                ax::mojom::Role::kParagraph};

static const ax::mojom::Role kRolesToSkip[] = {
    ax::mojom::Role::kAudio,
    ax::mojom::Role::kBanner,
    ax::mojom::Role::kButton,
    ax::mojom::Role::kComplementary,
    ax::mojom::Role::kContentInfo,
    ax::mojom::Role::kFooter,
    ax::mojom::Role::kFooterAsNonLandmark,
    ax::mojom::Role::kImage,
    ax::mojom::Role::kLabelText,
    ax::mojom::Role::kNavigation,
    /* input elements */
    ax::mojom::Role::kTextField,
    ax::mojom::Role::kTextFieldWithComboBox,
    ax::mojom::Role::kComboBoxSelect,
    ax::mojom::Role::kListBox,
    ax::mojom::Role::kListBoxOption,
    ax::mojom::Role::kCheckBox,
    ax::mojom::Role::kRadioButton,
    ax::mojom::Role::kSlider,
    ax::mojom::Role::kSpinButton,
    ax::mojom::Role::kSearchBox,
};

void GetContentRootNodes(const ui::AXNode* root,
                         std::vector<const ui::AXNode*>* content_root_nodes) {
  std::queue<const ui::AXNode*> queue;
  queue.push(root);
  while (!queue.empty()) {
    const ui::AXNode* node = queue.front();
    queue.pop();
    // If a main or article node is found, add it to the list of content root
    // nodes and continue. Do not explore children for nested article nodes.
    if (node->GetRole() == ax::mojom::Role::kMain ||
        node->GetRole() == ax::mojom::Role::kArticle) {
      content_root_nodes->push_back(node);
      continue;
    }
    for (auto iter = node->UnignoredChildrenBegin();
         iter != node->UnignoredChildrenEnd(); ++iter) {
      queue.push(iter.get());
    }
  }
}

void AddContentNodesToVector(const ui::AXNode* node,
                             std::vector<const ui::AXNode*>* content_nodes) {
  if (base::Contains(kContentRoles, node->GetRole())) {
    content_nodes->emplace_back(node);
    return;
  }
  if (base::Contains(kRolesToSkip, node->GetRole())) {
    return;
  }
  for (auto iter = node->UnignoredChildrenBegin();
       iter != node->UnignoredChildrenEnd(); ++iter) {
    AddContentNodesToVector(iter.get(), content_nodes);
  }
}

void AddTextNodesToVector(const ui::AXNode* node,
                          std::vector<std::u16string>* strings) {
  const ui::AXNodeData& node_data = node->data();

  if (base::Contains(kRolesToSkip, node_data.role)) {
    return;
  }

  if (node_data.role == ax::mojom::Role::kStaticText) {
    if (node_data.HasStringAttribute(ax::mojom::StringAttribute::kName)) {
      strings->push_back(
          node_data.GetString16Attribute(ax::mojom::StringAttribute::kName));
    }
    return;
  }

  for (const auto* child : node->children()) {
    AddTextNodesToVector(child, strings);
  }
}

[[maybe_unused]] std::vector<AXPropertyFilter> DefaultFilters() {
    std::vector<AXPropertyFilter> property_filters;

    property_filters.emplace_back("value='*'", AXPropertyFilter::ALLOW);
    // The value attribute on the document object contains the URL of the
    // current page which will not be the same every time the test is run.
    // The PDF plugin uses the 'chrome-extension' protocol, so block that as
    // well.
    property_filters.emplace_back("value='http*'", AXPropertyFilter::DENY);
    property_filters.emplace_back("value='chrome-extension*'",
                                  AXPropertyFilter::DENY);
    // Object attributes.value
    property_filters.emplace_back("layout-guess:*", AXPropertyFilter::ALLOW);

    property_filters.emplace_back("select*", AXPropertyFilter::ALLOW);
    property_filters.emplace_back("descript*", AXPropertyFilter::ALLOW);
    property_filters.emplace_back("check*", AXPropertyFilter::ALLOW);
    property_filters.emplace_back("horizontal", AXPropertyFilter::ALLOW);
    property_filters.emplace_back("multiselectable", AXPropertyFilter::ALLOW);
    property_filters.emplace_back("isPageBreakingObject*",
                                  AXPropertyFilter::ALLOW);

    // Deny most empty values
    property_filters.emplace_back("*=''", AXPropertyFilter::DENY);
    // After denying empty values, because we want to allow name=''
    property_filters.emplace_back("name=*", AXPropertyFilter::ALLOW_EMPTY);

    return property_filters;
  }

}  // namespace

void DistillPageText(
    content::RenderFrame* render_frame,
    base::OnceCallback<void(const absl::optional<std::string>&)> callback) {
  auto snapshotter = render_frame->CreateAXTreeSnapshotter(
      ui::AXMode::kWebContents | ui::AXMode::kHTML | ui::AXMode::kScreenReader);
  ui::AXTreeUpdate snapshot;
  snapshotter->Snapshot(
      /* max_nodes= */ 9000, /* timeout= */ base::Seconds(4), &snapshot);
  ui::AXTree tree(snapshot);

  LOG(ERROR) << base::CommandLine::ForCurrentProcess()->HasSwitch("pdf-renderer") << "\n";

  // // PDF
  // ui::AXPlatformNode::NotifyAddAXModeFlags(ui::kAXModeComplete);
  // auto snapper = render_frame->CreateAXTreeSnapshotter(ui::AXMode::kPDF);
  // ui::AXTreeUpdate snap;
  // snapper->Snapshot(/*max_node_count=*/0, /*timeout=*/{}, &snap);
  // ui::AXTree pdf_tree(snap);
  // std::unique_ptr<ui::AXTreeFormatter> formatter = content::AXInspectFactory::CreateBlinkFormatter();
  // formatter->SetPropertyFilters(DefaultFilters());
  // base::Value::Dict node = formatter->BuildTreeForNode(pdf_tree.root());
  // std::string contents = formatter->FormatTree(node);
  // LOG(ERROR) << contents << "\n";


  std::vector<const ui::AXNode*> content_root_nodes;
  std::vector<const ui::AXNode*> content_nodes;
  GetContentRootNodes(tree.root(), &content_root_nodes);

  for (const ui::AXNode* content_root_node : content_root_nodes) {
    AddContentNodesToVector(content_root_node, &content_nodes);
  }

  std::vector<std::u16string> text_node_contents;
  for (const ui::AXNode* content_node : content_nodes) {
    AddTextNodesToVector(content_node, &text_node_contents);
  }

  std::string contents_text =
      base::UTF16ToUTF8(base::JoinString(text_node_contents, u" "));

  if (contents_text.empty()) {
    std::move(callback).Run({});
    return;
  }
  std::move(callback).Run(contents_text);
}

}  // namespace ai_chat
