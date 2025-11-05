// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_ANNOTATED_PAGE_CONTENT_TEST_UTIL_H_
#define BRAVE_BROWSER_AI_CHAT_ANNOTATED_PAGE_CONTENT_TEST_UTIL_H_

#include <string>
#include <vector>

#include "components/optimization_guide/proto/features/common_quality_data.pb.h"

// Creates AnnotatedPageContent for testing
namespace ai_chat::annotated_page_content_test_util {

optimization_guide::proto::AnnotatedPageContent CreateMinimalPage(
    const std::string& title = "Test Page",
    const std::string& url = "https://example.com");

optimization_guide::proto::AnnotatedPageContent CreatePageWithViewport(
    int width = 1920,
    int height = 1080,
    int x = 0,
    int y = 0);

optimization_guide::proto::AnnotatedPageContent CreatePageWithContent(
    const optimization_guide::proto::ContentNode& root_content,
    const std::string& title = "Test Page",
    const std::string& url = "https://example.com");

optimization_guide::proto::AnnotatedPageContent CreateEmptyPage();
optimization_guide::proto::AnnotatedPageContent CreatePageWithoutRootNode();
optimization_guide::proto::AnnotatedPageContent
CreatePageWithComplexStructure();
optimization_guide::proto::AnnotatedPageContent CreatePageWithFormElements();
optimization_guide::proto::AnnotatedPageContent CreatePageWithTableStructure();
optimization_guide::proto::AnnotatedPageContent
CreatePageWithInteractiveElements();

// Builder for complex custom scenarios
class ContentNodeBuilder {
 public:
  ContentNodeBuilder() = default;
  ~ContentNodeBuilder() = default;

  ContentNodeBuilder& AsText(const std::string& text);
  ContentNodeBuilder& AsHeading(const std::string& text);
  ContentNodeBuilder& AsParagraph();
  ContentNodeBuilder& AsAnchor(const std::string& url, const std::string& text);
  ContentNodeBuilder& AsForm(const std::string& name);
  ContentNodeBuilder& AsFormControl(const std::string& name,
                                    const std::string& value = "",
                                    const std::string& placeholder = "");
  ContentNodeBuilder& AsImage(const std::string& alt_text);
  ContentNodeBuilder& AsTable(const std::string& name = "");
  ContentNodeBuilder& AsTableRow();
  ContentNodeBuilder& AsTableCell();
  ContentNodeBuilder& AsOrderedList();
  ContentNodeBuilder& AsUnorderedList();
  ContentNodeBuilder& AsListItem();
  ContentNodeBuilder& AsContainer();
  ContentNodeBuilder& AsIframe(const std::string& document_identifier);
  ContentNodeBuilder& AsSvg(const std::string& inner_text = "");
  ContentNodeBuilder& AsVideo(const std::string& url);
  ContentNodeBuilder& AsCanvas();
  ContentNodeBuilder& MakeClickable(int dom_id = 42);
  ContentNodeBuilder& MakeEditable(int dom_id = 42);
  ContentNodeBuilder& MakeScrollable(int dom_id,
                                     int content_width,
                                     int content_height,
                                     int visible_width,
                                     int visible_height,
                                     int visible_x = 0,
                                     int visible_y = 0,
                                     bool horizontal = true,
                                     bool vertical = true);
  ContentNodeBuilder& WithGeometry(int x, int y, int width, int height);
  ContentNodeBuilder& WithRole(optimization_guide::proto::AnnotatedRole role);
  ContentNodeBuilder& WithLabel(const std::string& label);
  ContentNodeBuilder& WithChildren(
      std::vector<optimization_guide::proto::ContentNode> children);

  optimization_guide::proto::ContentNode Build() { return std::move(node_); }

 private:
  optimization_guide::proto::ContentNode node_;
};

}  // namespace ai_chat::annotated_page_content_test_util

#endif  // BRAVE_BROWSER_AI_CHAT_ANNOTATED_PAGE_CONTENT_TEST_UTIL_H_
