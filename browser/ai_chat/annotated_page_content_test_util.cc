// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/annotated_page_content_test_util.h"

namespace ai_chat::annotated_page_content_test_util {

using optimization_guide::proto::AnnotatedPageContent;
using optimization_guide::proto::ContentNode;

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

AnnotatedPageContent CreatePageWithContent(const ContentNode& root_content,
                                           const std::string& title,
                                           const std::string& url) {
  auto page = CreateMinimalPage(title, url);
  auto* child = page.mutable_root_node()->add_children_nodes();
  child->CopyFrom(root_content);
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

  auto header = ContentNodeBuilder()
                    .AsHeading("Welcome")
                    .WithRole(optimization_guide::proto::ANNOTATED_ROLE_HEADER)
                    .Build();

  auto nav_link = ContentNodeBuilder()
                      .AsAnchor("https://example.com/nav", "Navigation")
                      .Build();
  auto nav_container =
      ContentNodeBuilder()
          .AsContainer()
          .WithRole(optimization_guide::proto::ANNOTATED_ROLE_NAV)
          .WithChildren({nav_link})
          .Build();

  auto main_text =
      ContentNodeBuilder().AsText("Main content goes here").Build();
  auto main_container =
      ContentNodeBuilder()
          .AsContainer()
          .WithRole(optimization_guide::proto::ANNOTATED_ROLE_MAIN)
          .WithChildren({main_text})
          .Build();

  auto root_container =
      ContentNodeBuilder()
          .AsContainer()
          .WithChildren({header, nav_container, main_container})
          .Build();
  *page.mutable_root_node() = root_container;
  return page;
}

AnnotatedPageContent CreatePageWithFormElements() {
  auto page = CreateMinimalPage("Form Page", "https://form.example.com");

  auto input1 = ContentNodeBuilder()
                    .AsFormControl("email", "", "Enter email")
                    .MakeEditable(101)
                    .Build();
  auto input2 = ContentNodeBuilder()
                    .AsFormControl("password", "", "Enter password")
                    .MakeEditable(102)
                    .Build();
  auto button = ContentNodeBuilder()
                    .AsText("Submit")
                    .MakeClickable(103)
                    .WithGeometry(50, 200, 100, 30)
                    .Build();

  auto form = ContentNodeBuilder()
                  .AsForm("loginform")
                  .WithChildren({input1, input2, button})
                  .Build();
  *page.mutable_root_node() = form;
  return page;
}

AnnotatedPageContent CreatePageWithTableStructure() {
  auto page = CreateMinimalPage("Table Page", "https://table.example.com");

  auto cell1 =
      ContentNodeBuilder()
          .AsTableCell()
          .WithChildren({ContentNodeBuilder().AsText("Header 1").Build()})
          .Build();
  auto cell2 =
      ContentNodeBuilder()
          .AsTableCell()
          .WithChildren({ContentNodeBuilder().AsText("Header 2").Build()})
          .Build();
  auto row1 =
      ContentNodeBuilder().AsTableRow().WithChildren({cell1, cell2}).Build();

  auto cell3 =
      ContentNodeBuilder()
          .AsTableCell()
          .WithChildren({ContentNodeBuilder().AsText("Data 1").Build()})
          .Build();
  auto cell4 =
      ContentNodeBuilder()
          .AsTableCell()
          .WithChildren({ContentNodeBuilder().AsText("Data 2").Build()})
          .Build();
  auto row2 =
      ContentNodeBuilder().AsTableRow().WithChildren({cell3, cell4}).Build();

  auto table = ContentNodeBuilder()
                   .AsTable("data_table")
                   .WithChildren({row1, row2})
                   .Build();
  *page.mutable_root_node() = table;
  return page;
}

AnnotatedPageContent CreatePageWithInteractiveElements() {
  auto page =
      CreateMinimalPage("Interactive Page", "https://interactive.example.com");

  auto button = ContentNodeBuilder()
                    .AsText("Click Me")
                    .MakeClickable(201)
                    .WithGeometry(10, 10, 100, 30)
                    .Build();

  auto input = ContentNodeBuilder()
                   .AsFormControl("search", "", "Search here")
                   .MakeEditable(202)
                   .WithGeometry(10, 50, 300, 25)
                   .Build();

  auto link = ContentNodeBuilder()
                  .AsAnchor("https://example.com", "Link")
                  .MakeClickable(203)
                  .WithGeometry(10, 100, 100, 20)
                  .Build();

  auto container = ContentNodeBuilder()
                       .AsContainer()
                       .WithChildren({button, input, link})
                       .Build();
  *page.mutable_root_node() = container;
  return page;
}

// ContentNodeBuilder implementations
ContentNodeBuilder& ContentNodeBuilder::AsText(const std::string& text) {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_TEXT);
  node_.mutable_content_attributes()->mutable_text_data()->set_text_content(
      text);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsHeading(const std::string& text) {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_HEADING);
  node_.mutable_content_attributes()->mutable_text_data()->set_text_content(
      text);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsParagraph() {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_PARAGRAPH);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsAnchor(const std::string& url,
                                                 const std::string& text) {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_ANCHOR);
  node_.mutable_content_attributes()->mutable_anchor_data()->set_url(url);
  if (!text.empty()) {
    *node_.add_children_nodes() = ContentNodeBuilder().AsText(text).Build();
  }
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsForm(const std::string& name) {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_FORM);
  node_.mutable_content_attributes()->mutable_form_data()->set_form_name(name);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsFormControl(
    const std::string& name,
    const std::string& value,
    const std::string& placeholder) {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_FORM_CONTROL);
  auto* control_data =
      node_.mutable_content_attributes()->mutable_form_control_data();
  control_data->set_field_name(name);
  if (!value.empty()) {
    control_data->set_field_value(value);
  }
  if (!placeholder.empty()) {
    control_data->set_placeholder(placeholder);
  }
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsImage(const std::string& alt_text) {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_IMAGE);
  node_.mutable_content_attributes()->mutable_image_data()->set_image_caption(
      alt_text);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsTable(const std::string& name) {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_TABLE);
  if (!name.empty()) {
    node_.mutable_content_attributes()->mutable_table_data()->set_table_name(
        name);
  }
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsTableRow() {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_TABLE_ROW);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsTableCell() {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_TABLE_CELL);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsOrderedList() {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_ORDERED_LIST);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsUnorderedList() {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_UNORDERED_LIST);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsListItem() {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_LIST_ITEM);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsContainer() {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_CONTAINER);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsIframe(
    const std::string& document_identifier) {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_IFRAME);
  node_.mutable_content_attributes()
      ->mutable_iframe_data()
      ->mutable_frame_data()
      ->mutable_document_identifier()
      ->set_serialized_token(document_identifier);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsSvg(const std::string& inner_text) {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_SVG);
  if (!inner_text.empty()) {
    node_.mutable_content_attributes()->mutable_svg_data()->set_inner_text(
        inner_text);
  }
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsVideo(const std::string& url) {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_VIDEO);
  node_.mutable_content_attributes()->mutable_video_data()->set_url(url);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::AsCanvas() {
  node_.mutable_content_attributes()->set_attribute_type(
      optimization_guide::proto::CONTENT_ATTRIBUTE_CANVAS);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::MakeClickable(int dom_id) {
  auto* attrs = node_.mutable_content_attributes();
  attrs->set_common_ancestor_dom_node_id(dom_id);
  attrs->mutable_interaction_info()->set_is_clickable(true);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::MakeEditable(int dom_id) {
  auto* attrs = node_.mutable_content_attributes();
  attrs->set_common_ancestor_dom_node_id(dom_id);
  attrs->mutable_interaction_info()->set_is_editable(true);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::MakeScrollable(int dom_id,
                                                       int content_width,
                                                       int content_height,
                                                       int visible_width,
                                                       int visible_height,
                                                       int visible_x,
                                                       int visible_y,
                                                       bool horizontal,
                                                       bool vertical) {
  auto* attrs = node_.mutable_content_attributes();
  attrs->set_common_ancestor_dom_node_id(dom_id);
  auto* scroller_info =
      attrs->mutable_interaction_info()->mutable_scroller_info();
  scroller_info->set_user_scrollable_horizontal(horizontal);
  scroller_info->set_user_scrollable_vertical(vertical);

  auto* scrolling_bounds = scroller_info->mutable_scrolling_bounds();
  scrolling_bounds->set_width(content_width);
  scrolling_bounds->set_height(content_height);

  auto* visible_area = scroller_info->mutable_visible_area();
  visible_area->set_width(visible_width);
  visible_area->set_height(visible_height);
  visible_area->set_x(visible_x);
  visible_area->set_y(visible_y);

  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::WithGeometry(int x,
                                                     int y,
                                                     int width,
                                                     int height) {
  auto* attrs = node_.mutable_content_attributes();
  auto* bbox = attrs->mutable_geometry()->mutable_outer_bounding_box();
  bbox->set_x(x);
  bbox->set_y(y);
  bbox->set_width(width);
  bbox->set_height(height);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::WithRole(
    optimization_guide::proto::AnnotatedRole role) {
  auto* attrs = node_.mutable_content_attributes();
  attrs->add_annotated_roles(role);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::WithLabel(const std::string& label) {
  auto* attrs = node_.mutable_content_attributes();
  attrs->set_label(label);
  return *this;
}

ContentNodeBuilder& ContentNodeBuilder::WithChildren(
    std::vector<optimization_guide::proto::ContentNode> children) {
  for (auto& child : children) {
    *node_.add_children_nodes() = std::move(child);
  }
  return *this;
}

}  // namespace ai_chat::annotated_page_content_test_util
