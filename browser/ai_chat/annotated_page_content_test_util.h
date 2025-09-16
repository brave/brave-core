// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_ANNOTATED_PAGE_CONTENT_TEST_UTIL_H_
#define BRAVE_BROWSER_AI_CHAT_ANNOTATED_PAGE_CONTENT_TEST_UTIL_H_

#include <string>
#include <vector>

#include "components/optimization_guide/proto/features/common_quality_data.pb.h"

namespace ai_chat::annotated_page_content_test_util {

// Basic page builders with sensible defaults
optimization_guide::proto::AnnotatedPageContent CreateMinimalPage(
    const std::string& title = "Test Page",
    const std::string& url = "https://example.com");

optimization_guide::proto::AnnotatedPageContent CreatePageWithViewport(
    int width = 1920,
    int height = 1080,
    int x = 0,
    int y = 0);

// Basic content node builders
optimization_guide::proto::ContentNode CreateTextNode(
    const std::string& text = "Sample text");

optimization_guide::proto::ContentNode CreateHeadingNode(
    const std::string& text = "Heading");

optimization_guide::proto::ContentNode CreateParagraphNode(
    std::vector<optimization_guide::proto::ContentNode> children = {});

optimization_guide::proto::ContentNode CreateAnchorNode(
    const std::string& url = "https://example.com",
    const std::string& text = "Link text");

optimization_guide::proto::ContentNode CreateImageNode(
    const std::string& alt_text = "Image");

optimization_guide::proto::ContentNode CreateFormNode(
    const std::string& form_name = "testform",
    std::vector<optimization_guide::proto::ContentNode> inputs = {});

optimization_guide::proto::ContentNode CreateFormControlNode(
    const std::string& field_name = "field",
    const std::string& field_value = "",
    const std::string& placeholder = "");

optimization_guide::proto::ContentNode CreateTableNode(
    const std::string& table_name = "table",
    std::vector<optimization_guide::proto::ContentNode> rows = {});

optimization_guide::proto::ContentNode CreateTableRowNode(
    std::vector<optimization_guide::proto::ContentNode> cells = {});

optimization_guide::proto::ContentNode CreateTableCellNode(
    std::vector<optimization_guide::proto::ContentNode> content = {});

optimization_guide::proto::ContentNode CreateListNode(
    bool ordered = false,
    std::vector<optimization_guide::proto::ContentNode> items = {});

optimization_guide::proto::ContentNode CreateListItemNode(
    std::vector<optimization_guide::proto::ContentNode> content = {});

optimization_guide::proto::ContentNode CreateContainerNode(
    std::vector<optimization_guide::proto::ContentNode> children = {});

optimization_guide::proto::ContentNode CreateIframeNode(
    const std::string& document_identifier = "iframe123",
    std::vector<optimization_guide::proto::ContentNode> children = {});

optimization_guide::proto::ContentNode CreateSvgNode(
    const std::string& inner_text = "");

optimization_guide::proto::ContentNode CreateVideoNode(
    const std::string& url = "https://example.com/video.mp4");

optimization_guide::proto::ContentNode CreateCanvasNode();

// Interactive element modifiers
void MakeClickable(optimization_guide::proto::ContentNode& node,
                   int dom_id = 42);

void MakeEditable(optimization_guide::proto::ContentNode& node,
                  int dom_id = 42);

void AddGeometry(optimization_guide::proto::ContentNode& node,
                 int x = 100,
                 int y = 50,
                 int width = 200,
                 int height = 30);

void AddRole(optimization_guide::proto::ContentNode& node,
             optimization_guide::proto::AnnotatedRole role);

void AddLabel(optimization_guide::proto::ContentNode& node,
              const std::string& label);

// Common combinations for convenience
optimization_guide::proto::ContentNode CreateClickableButton(
    const std::string& text = "Click me",
    int dom_id = 42,
    int x = 100,
    int y = 50);

optimization_guide::proto::ContentNode CreateEditableInput(
    const std::string& name = "input",
    const std::string& placeholder = "Enter text",
    int dom_id = 42);

optimization_guide::proto::AnnotatedPageContent CreatePageWithContent(
    const optimization_guide::proto::ContentNode& root_content,
    const std::string& title = "Test Page",
    const std::string& url = "https://example.com");

// For testing specific scenarios
optimization_guide::proto::AnnotatedPageContent CreateEmptyPage();
optimization_guide::proto::AnnotatedPageContent CreatePageWithoutRootNode();
optimization_guide::proto::AnnotatedPageContent
CreatePageWithComplexStructure();
optimization_guide::proto::AnnotatedPageContent CreatePageWithFormElements();
optimization_guide::proto::AnnotatedPageContent CreatePageWithTableStructure();
optimization_guide::proto::AnnotatedPageContent
CreatePageWithInteractiveElements();

// Builder for complex custom scenarios (inline implementation)
class ContentNodeBuilder {
 public:
  ContentNodeBuilder() = default;
  ~ContentNodeBuilder() = default;

  ContentNodeBuilder& WithText(const std::string& text) {
    node_.mutable_content_attributes()->set_attribute_type(
        optimization_guide::proto::CONTENT_ATTRIBUTE_TEXT);
    node_.mutable_content_attributes()->mutable_text_data()->set_text_content(
        text);
    return *this;
  }

  ContentNodeBuilder& AsHeading(const std::string& text) {
    node_.mutable_content_attributes()->set_attribute_type(
        optimization_guide::proto::CONTENT_ATTRIBUTE_HEADING);
    node_.mutable_content_attributes()->mutable_text_data()->set_text_content(
        text);
    return *this;
  }

  ContentNodeBuilder& AsParagraph() {
    node_.mutable_content_attributes()->set_attribute_type(
        optimization_guide::proto::CONTENT_ATTRIBUTE_PARAGRAPH);
    return *this;
  }

  ContentNodeBuilder& AsAnchor(const std::string& url,
                               const std::string& text) {
    node_.mutable_content_attributes()->set_attribute_type(
        optimization_guide::proto::CONTENT_ATTRIBUTE_ANCHOR);
    node_.mutable_content_attributes()->mutable_anchor_data()->set_url(url);
    node_.mutable_content_attributes()->mutable_text_data()->set_text_content(
        text);
    return *this;
  }

  ContentNodeBuilder& AsForm(const std::string& name) {
    node_.mutable_content_attributes()->set_attribute_type(
        optimization_guide::proto::CONTENT_ATTRIBUTE_FORM);
    node_.mutable_content_attributes()->mutable_form_data()->set_form_name(
        name);
    return *this;
  }

  ContentNodeBuilder& AsFormControl(const std::string& name,
                                    const std::string& value = "",
                                    const std::string& placeholder = "") {
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

  ContentNodeBuilder& AsContainer() {
    node_.mutable_content_attributes()->set_attribute_type(
        optimization_guide::proto::CONTENT_ATTRIBUTE_CONTAINER);
    return *this;
  }

  ContentNodeBuilder& MakeClickable(int dom_id = 42) {
    auto* attrs = node_.mutable_content_attributes();
    attrs->set_common_ancestor_dom_node_id(dom_id);
    attrs->mutable_interaction_info()->set_is_clickable(true);
    return *this;
  }

  ContentNodeBuilder& WithGeometry(int x, int y, int width, int height) {
    auto* attrs = node_.mutable_content_attributes();
    auto* bbox = attrs->mutable_geometry()->mutable_outer_bounding_box();
    bbox->set_x(x);
    bbox->set_y(y);
    bbox->set_width(width);
    bbox->set_height(height);
    return *this;
  }

  ContentNodeBuilder& WithRole(optimization_guide::proto::AnnotatedRole role) {
    auto* attrs = node_.mutable_content_attributes();
    attrs->add_annotated_roles(role);
    return *this;
  }

  ContentNodeBuilder& WithLabel(const std::string& label) {
    auto* attrs = node_.mutable_content_attributes();
    attrs->set_label(label);
    return *this;
  }

  ContentNodeBuilder& WithChildren(
      std::vector<optimization_guide::proto::ContentNode> children) {
    for (auto& child : children) {
      *node_.add_children_nodes() = std::move(child);
    }
    return *this;
  }

  optimization_guide::proto::ContentNode Build() { return std::move(node_); }

 private:
  optimization_guide::proto::ContentNode node_;
};

}  // namespace ai_chat::annotated_page_content_test_util

#endif  // BRAVE_BROWSER_AI_CHAT_ANNOTATED_PAGE_CONTENT_TEST_UTIL_H_
