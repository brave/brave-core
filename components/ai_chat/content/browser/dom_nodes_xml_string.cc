// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/dom_nodes_xml_string.h"

#include "base/logging.h"
#include "ui/accessibility/ax_enums.mojom-shared.h"
#include "ui/accessibility/ax_mode.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/ax_role_properties.h"
#include "ui/accessibility/ax_tree.h"
#include "ui/accessibility/ax_tree_id.h"
#include "ui/accessibility/ax_tree_manager.h"
#include "ui/accessibility/platform/browser_accessibility_manager.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_enum_util.cc"

namespace {

std::string GetRoleString(ax::mojom::Role role) {
  if (role == ax::mojom::Role::kRootWebArea)
    return "root";
  if (role == ax::mojom::Role::kStaticText)
    return "text";

  return ui::ToString(role);
}

std::string EscapeXml(const std::string& input) {
  std::string output = input;
  base::ReplaceChars(output, "&", "&amp;", &output);
  base::ReplaceChars(output, "<", "&lt;", &output);
  base::ReplaceChars(output, ">", "&gt;", &output);
  base::ReplaceChars(output, "\"", "&quot;", &output);
  base::ReplaceChars(output, "'", "&apos;", &output);
  return output;
}

class DomNodesXmlStringSerializer {
 public:
  explicit DomNodesXmlStringSerializer(ui::AXTreeUpdate& tree) : tree_(tree) {}

  std::string Serialize() {
    xml_ = "<dom-nodes>\n";

  // Find root nodes (nodes with no parents) and process them
  std::set<int32_t> all_child_ids;
  for (const auto& node : tree_->nodes) {
    LOG(ERROR) << "node: " << node.id;
    for (int32_t child_id : node.child_ids) {
      all_child_ids.insert(child_id);
    }
  }

  for (const auto& node : tree_->nodes) {
    if (all_child_ids.find(node.id) == all_child_ids.end()) {
      // This is a root node
      BuildXml(node, 1, false);
    }
  }

  xml_ += "</dom-nodes>";

  LOG(ERROR) << "XML: " << xml_;
  return xml_;
}

 private:
  void BuildXml(const ui::AXNodeData& data, int depth, bool inside_control) {
    std::string indent(depth * 2, ' ');
    std::string role = GetRoleString(data.role);

    LOG(ERROR) << "Processing node: " << data.id;

    bool is_interesting = IsInteresting(data, inside_control);

    if (is_interesting) {
      xml_ += indent + "<" + role;

      // Add ID for interactive elements
      if (data.IsClickable() || data.HasState(ax::mojom::State::kFocusable)) {
        if (data.id != ui::AXNodeData::kInvalidAXID) {
          xml_ += " id=\"" + EscapeXml(base::NumberToString(data.id)) + "\"";
        }
      }

      // Add name if present
      auto name = data.GetStringAttribute(ax::mojom::StringAttribute::kName);
      if (!name.empty()) {
        xml_ += " name=\"" + EscapeXml(name) + "\"";
      }

      // Add value for form controls
      auto value = data.GetStringAttribute(ax::mojom::StringAttribute::kValue);
      if (data.HasState(ax::mojom::State::kEditable) && !value.empty()) {
        xml_ += " value=\"" + EscapeXml(value) + "\"";
      }

      // Add href for links
      if (data.role == ax::mojom::Role::kLink &&
          !data.GetStringAttribute(ax::mojom::StringAttribute::kUrl).empty()) {
        xml_ += " href=\"" +
              EscapeXml(data.GetStringAttribute(ax::mojom::StringAttribute::kUrl)) +
              "\"";
      }
      xml_ += data.child_ids.empty() ? "/>\n" : ">\n";
    } else {
      LOG(ERROR) << "Ignoring node: " << data.id << " (" << role << ")";
    }

    if (!data.child_ids.empty()) {
      // Process all children
      for (int32_t child_id : data.child_ids) {
        // Find child node data
        auto child_it = std::find_if(tree_->nodes.begin(), tree_->nodes.end(),
                                    [child_id](const ui::AXNodeData& node) {
                                      return node.id == child_id;
                                    });
        if (child_it != tree_->nodes.end()) {
          LOG(ERROR) << "child: " << child_id;
          BuildXml(*child_it, is_interesting ? depth + 1 : depth, ui::IsControl(data.role));
        } else {
          LOG(ERROR) << "Child node not found: " << child_id;
        }
      }

      if (is_interesting) {
        xml_ += indent + "</" + role + ">\n";
      }
    }
  }

  bool IsInteresting(const ui::AXNodeData& data, bool inside_control) {
    if (data.IsIgnored()) {
      return false;
    }

    if (data.role == ax::mojom::Role::kGenericContainer &&
        !data.HasStringAttribute(ax::mojom::StringAttribute::kName)) {
      return false;
    }

    if (data.IsActivatable() || data.IsClickable()) {
      return true;
    }

    if (ui::IsControl(data.role)) {
      return true;
    }

    // A non focusable child of a control is not interesting
    // if (inside_control) {
    //   return false;
    // }

    auto name = data.GetStringAttribute(ax::mojom::StringAttribute::kName);

    return IsLeafNode(data) && !name.empty();
  }

  bool IsLeafNode(const ui::AXNodeData& data) {
    if (data.child_ids.empty()) {
      return true;
    }

    if (ui::IsTextField(data.role) || ui::IsText(data.role)) {
      return true;
    }

    // TODO(petemill): check presentational items, e.g. image / separator /
    // progressbar.

    return false;
  }

  raw_ref<ui::AXTreeUpdate> tree_;
  std::string xml_;
};

}  // namespace

namespace ai_chat {

std::string GetDomNodesXmlString(ui::AXTreeUpdate& tree) {
  return DomNodesXmlStringSerializer(tree).Serialize();
}

}  // namespace ai_chat
