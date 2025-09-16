// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/annotated_page_content_test_util.h"

namespace ai_chat::annotated_page_content_test_util {

using optimization_guide::proto::AnnotatedPageContent;
using optimization_guide::proto::BoundingRect;
using optimization_guide::proto::ContentAttributes;
using optimization_guide::proto::ContentAttributeType;
using optimization_guide::proto::ContentNode;
using optimization_guide::proto::FrameData;

AnnotatedPageContent CreateMinimalPage(const std::string& title,
                                       const std::string& url) {
  AnnotatedPageContent page;
  page.set_version(
      optimization_guide::proto::ANNOTATED_PAGE_CONTENT_VERSION_1_0);
  page.set_mode(optimization_guide::proto::ANNOTATED_PAGE_CONTENT_MODE_DEFAULT);

  // Add main frame data
  auto* frame_data = page.mutable_main_frame_data();
  frame_data->set_title(title);
  frame_data->set_url(url);
  frame_data->mutable_document_identifier()->set_serialized_token("main_doc");

  // Add viewport
  auto* viewport = page.mutable_viewport_geometry();
  viewport->set_width(1920);
  viewport->set_height(1080);
  viewport->set_x(0);
  viewport->set_y(0);

  // Add root node
  auto* root = page.mutable_root_node();
  root->mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_ROOT);

  return page;
}

AnnotatedPageContent CreatePageWithViewport(int width,
                                            int height,
                                            int x,
                                            int y) {
  auto page = CreateMinimalPage("Test Page", "https://example.com");
  auto* viewport = page.mutable_viewport_geometry();
  viewport->set_width(width);
  viewport->set_height(height);
  viewport->set_x(x);
  viewport->set_y(y);
  return page;
}

ContentNode CreateTextNode(const std::string& text) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(optimization_guide::proto::CONTENT_ATTRIBUTE_TEXT);
  attrs->mutable_text_data()->set_text_content(text);
  return node;
}

ContentNode CreateHeadingNode(const std::string& text) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_HEADING);
  attrs->mutable_text_data()->set_text_content(text);
  return node;
}

ContentNode CreateParagraphNode(std::vector<ContentNode> children) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_PARAGRAPH);

  for (auto& child : children) {
    *node.add_children_nodes() = std::move(child);
  }
  return node;
}

ContentNode CreateAnchorNode(const std::string& url, const std::string& text) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_ANCHOR);
  attrs->mutable_anchor_data()->set_url(url);
  attrs->mutable_text_data()->set_text_content(text);
  return node;
}

ContentNode CreateImageNode(const std::string& alt_text) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(optimization_guide::proto::CONTENT_ATTRIBUTE_IMAGE);
  attrs->mutable_image_data()->set_image_caption(alt_text);
  return node;
}

ContentNode CreateFormNode(const std::string& form_name,
                           std::vector<ContentNode> inputs) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(optimization_guide::proto::CONTENT_ATTRIBUTE_FORM);
  attrs->mutable_form_data()->set_form_name(form_name);

  for (auto& input : inputs) {
    *node.add_children_nodes() = std::move(input);
  }
  return node;
}

ContentNode CreateFormControlNode(const std::string& field_name,
                                  const std::string& field_value,
                                  const std::string& placeholder) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_FORM_CONTROL);
  auto* control_data = attrs->mutable_form_control_data();
  control_data->set_field_name(field_name);
  if (!field_value.empty()) {
    control_data->set_field_value(field_value);
  }
  if (!placeholder.empty()) {
    control_data->set_placeholder(placeholder);
  }
  return node;
}

ContentNode CreateTableNode(const std::string& table_name,
                            std::vector<ContentNode> rows) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(optimization_guide::proto::CONTENT_ATTRIBUTE_TABLE);
  attrs->mutable_table_data()->set_table_name(table_name);

  for (auto& row : rows) {
    *node.add_children_nodes() = std::move(row);
  }
  return node;
}

ContentNode CreateTableRowNode(std::vector<ContentNode> cells) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_TABLE_ROW);

  for (auto& cell : cells) {
    *node.add_children_nodes() = std::move(cell);
  }
  return node;
}

ContentNode CreateTableCellNode(std::vector<ContentNode> content) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_TABLE_CELL);

  for (auto& item : content) {
    *node.add_children_nodes() = std::move(item);
  }
  return node;
}

ContentNode CreateListNode(bool ordered, std::vector<ContentNode> items) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(
      ordered ? optimization_guide::proto::CONTENT_ATTRIBUTE_ORDERED_LIST
              : optimization_guide::proto::CONTENT_ATTRIBUTE_UNORDERED_LIST);

  for (auto& item : items) {
    *node.add_children_nodes() = std::move(item);
  }
  return node;
}

ContentNode CreateListItemNode(std::vector<ContentNode> content) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_LIST_ITEM);

  for (auto& item : content) {
    *node.add_children_nodes() = std::move(item);
  }
  return node;
}

ContentNode CreateContainerNode(std::vector<ContentNode> children) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_CONTAINER);

  for (auto& child : children) {
    *node.add_children_nodes() = std::move(child);
  }
  return node;
}

ContentNode CreateIframeNode(const std::string& document_identifier,
                             std::vector<ContentNode> children) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_IFRAME);
  attrs->mutable_iframe_data()
      ->mutable_frame_data()
      ->mutable_document_identifier()
      ->set_serialized_token(document_identifier);

  for (auto& child : children) {
    *node.add_children_nodes() = std::move(child);
  }
  return node;
}

ContentNode CreateSvgNode(const std::string& inner_text) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(optimization_guide::proto::CONTENT_ATTRIBUTE_SVG);
  if (!inner_text.empty()) {
    attrs->mutable_svg_data()->set_inner_text(inner_text);
  }
  return node;
}

ContentNode CreateVideoNode(const std::string& url) {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(optimization_guide::proto::CONTENT_ATTRIBUTE_VIDEO);
  attrs->mutable_video_data()->set_url(url);
  return node;
}

ContentNode CreateCanvasNode() {
  ContentNode node;
  auto* attrs = node.mutable_content_attributes();
  attrs->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_CANVAS);
  return node;
}

void MakeClickable(ContentNode& node, int dom_id) {
  auto* attrs = node.mutable_content_attributes();
  attrs->set_common_ancestor_dom_node_id(dom_id);
  attrs->mutable_interaction_info()->set_is_clickable(true);
}

void MakeEditable(ContentNode& node, int dom_id) {
  auto* attrs = node.mutable_content_attributes();
  attrs->set_common_ancestor_dom_node_id(dom_id);
  attrs->mutable_interaction_info()->set_is_editable(true);
}

void AddGeometry(ContentNode& node, int x, int y, int width, int height) {
  auto* attrs = node.mutable_content_attributes();
  auto* bbox = attrs->mutable_geometry()->mutable_outer_bounding_box();
  bbox->set_x(x);
  bbox->set_y(y);
  bbox->set_width(width);
  bbox->set_height(height);
}

void AddRole(ContentNode& node, optimization_guide::proto::AnnotatedRole role) {
  auto* attrs = node.mutable_content_attributes();
  attrs->add_annotated_roles(role);
}

void AddLabel(ContentNode& node, const std::string& label) {
  auto* attrs = node.mutable_content_attributes();
  attrs->set_label(label);
}

ContentNode CreateClickableButton(const std::string& text,
                                  int dom_id,
                                  int x,
                                  int y) {
  auto node = CreateTextNode(text);
  MakeClickable(node, dom_id);
  AddGeometry(node, x, y, 100, 30);
  return node;
}

ContentNode CreateEditableInput(const std::string& name,
                                const std::string& placeholder,
                                int dom_id) {
  auto node = CreateFormControlNode(name, "", placeholder);
  MakeEditable(node, dom_id);
  return node;
}

AnnotatedPageContent CreatePageWithContent(const ContentNode& root_content,
                                           const std::string& title,
                                           const std::string& url) {
  auto page = CreateMinimalPage(title, url);
  *page.mutable_root_node() = root_content;
  return page;
}

AnnotatedPageContent CreateEmptyPage() {
  AnnotatedPageContent page;
  return page;
}

AnnotatedPageContent CreatePageWithoutRootNode() {
  auto page = CreateMinimalPage("Test Page", "https://example.com");
  page.clear_root_node();
  return page;
}

AnnotatedPageContent CreatePageWithComplexStructure() {
  auto page = CreateMinimalPage("Complex Page", "https://complex.example.com");

  auto header = CreateHeadingNode("Welcome");
  AddRole(header, optimization_guide::proto::ANNOTATED_ROLE_HEADER);

  auto nav_link = CreateAnchorNode("https://example.com/nav", "Navigation");
  auto nav_container = CreateContainerNode({nav_link});
  AddRole(nav_container, optimization_guide::proto::ANNOTATED_ROLE_NAV);

  auto main_text = CreateTextNode("Main content goes here");
  auto main_container = CreateContainerNode({main_text});
  AddRole(main_container, optimization_guide::proto::ANNOTATED_ROLE_MAIN);

  auto root_container =
      CreateContainerNode({header, nav_container, main_container});
  *page.mutable_root_node() = root_container;
  return page;
}

AnnotatedPageContent CreatePageWithFormElements() {
  auto page = CreateMinimalPage("Form Page", "https://form.example.com");

  std::vector<ContentNode> inputs;
  inputs.push_back(CreateEditableInput("email", "Enter email", 101));
  inputs.push_back(CreateEditableInput("password", "Enter password", 102));
  inputs.push_back(CreateClickableButton("Submit", 103, 50, 200));

  auto form = CreateFormNode("loginform", std::move(inputs));
  *page.mutable_root_node() = form;
  return page;
}

AnnotatedPageContent CreatePageWithTableStructure() {
  auto page = CreateMinimalPage("Table Page", "https://table.example.com");

  std::vector<ContentNode> cells;
  cells.push_back(CreateTableCellNode({CreateTextNode("Header 1")}));
  cells.push_back(CreateTableCellNode({CreateTextNode("Header 2")}));

  std::vector<ContentNode> rows;
  rows.push_back(CreateTableRowNode(std::move(cells)));

  std::vector<ContentNode> data_cells;
  data_cells.push_back(CreateTableCellNode({CreateTextNode("Data 1")}));
  data_cells.push_back(CreateTableCellNode({CreateTextNode("Data 2")}));
  rows.push_back(CreateTableRowNode(std::move(data_cells)));

  auto table = CreateTableNode("data_table", std::move(rows));
  *page.mutable_root_node() = table;
  return page;
}

AnnotatedPageContent CreatePageWithInteractiveElements() {
  auto page =
      CreateMinimalPage("Interactive Page", "https://interactive.example.com");

  std::vector<ContentNode> elements;
  elements.push_back(CreateClickableButton("Click Me", 201, 10, 10));

  auto editable_input = CreateEditableInput("search", "Search here", 202);
  AddGeometry(editable_input, 10, 50, 300, 25);
  elements.push_back(editable_input);

  auto clickable_link = CreateAnchorNode("https://example.com", "Link");
  MakeClickable(clickable_link, 203);
  AddGeometry(clickable_link, 10, 100, 100, 20);
  elements.push_back(clickable_link);

  auto container = CreateContainerNode(std::move(elements));
  *page.mutable_root_node() = container;
  return page;
}

// ContentNodeBuilder implementation is in header file

}  // namespace ai_chat::annotated_page_content_test_util
