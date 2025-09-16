// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/page_content_blocks.h"

#include <string>

#include "base/strings/string_util.h"
#include "brave/browser/ai_chat/annotated_page_content_test_util.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/optimization_guide/proto/features/common_quality_data.pb.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

// Helper matchers for string content
#define EXPECT_CONTAINS(str, substr) \
  EXPECT_THAT(str, testing::HasSubstr(substr))
#define EXPECT_NOT_CONTAINS(str, substr) \
  EXPECT_THAT(str, testing::Not(testing::HasSubstr(substr)))
using annotated_page_content_test_util::AddGeometry;
using annotated_page_content_test_util::AddLabel;
using annotated_page_content_test_util::AddRole;
using annotated_page_content_test_util::ContentNodeBuilder;
using annotated_page_content_test_util::CreateAnchorNode;
using annotated_page_content_test_util::CreateCanvasNode;
using annotated_page_content_test_util::CreateClickableButton;
using annotated_page_content_test_util::CreateContainerNode;
using annotated_page_content_test_util::CreateEditableInput;
using annotated_page_content_test_util::CreateEmptyPage;
using annotated_page_content_test_util::CreateFormControlNode;
using annotated_page_content_test_util::CreateFormNode;
using annotated_page_content_test_util::CreateHeadingNode;
using annotated_page_content_test_util::CreateIframeNode;
using annotated_page_content_test_util::CreateImageNode;
using annotated_page_content_test_util::CreateListItemNode;
using annotated_page_content_test_util::CreateListNode;
using annotated_page_content_test_util::CreateMinimalPage;
using annotated_page_content_test_util::CreatePageWithComplexStructure;
using annotated_page_content_test_util::CreatePageWithContent;
using annotated_page_content_test_util::CreatePageWithFormElements;
using annotated_page_content_test_util::CreatePageWithInteractiveElements;
using annotated_page_content_test_util::CreatePageWithoutRootNode;
using annotated_page_content_test_util::CreatePageWithTableStructure;
using annotated_page_content_test_util::CreatePageWithViewport;
using annotated_page_content_test_util::CreateParagraphNode;
using annotated_page_content_test_util::CreateSvgNode;
using annotated_page_content_test_util::CreateTableCellNode;
using annotated_page_content_test_util::CreateTableNode;
using annotated_page_content_test_util::CreateTableRowNode;
using annotated_page_content_test_util::CreateTextNode;
using annotated_page_content_test_util::CreateVideoNode;
using annotated_page_content_test_util::MakeClickable;
using annotated_page_content_test_util::MakeEditable;

class PageContentBlocksTest : public testing::Test {
 protected:
  std::string GetContentText(
      const std::vector<mojom::ContentBlockPtr>& blocks) {
    if (blocks.empty() || !blocks[0]->is_text_content_block()) {
      return "";
    }
    return blocks[0]->get_text_content_block()->text;
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
};

// ===== INPUT VALIDATION & EDGE CASES =====

TEST_F(PageContentBlocksTest, ConvertEmptyPageContent) {
  auto empty_page = CreateEmptyPage();
  auto result = ConvertAnnotatedPageContentToBlocks(empty_page);

  EXPECT_EQ(result.size(), 0u);
}

TEST_F(PageContentBlocksTest, ConvertPageContentWithoutRootNode) {
  auto page = CreatePageWithoutRootNode();
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  EXPECT_EQ(result.size(), 0u);
}

TEST_F(PageContentBlocksTest, ConvertPageContentWithEmptyRootNode) {
  auto page = CreateMinimalPage();
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_EQ(result.size(), 1u);
  EXPECT_CONTAINS(content, "=== PAGE METADATA ===");
  EXPECT_CONTAINS(content, "PAGE TITLE: Test Page");
  EXPECT_CONTAINS(content, "PAGE URL: https://example.com");
  EXPECT_CONTAINS(content, "=== PAGE STRUCTURE (XML representation) ===");
  EXPECT_CONTAINS(content, "=== INTERACTION INSTRUCTIONS ===");
}

// ===== PAGE METADATA EXTRACTION TESTS =====

TEST_F(PageContentBlocksTest, ExtractCompletePageMetadata) {
  auto page =
      CreateMinimalPage("My Custom Title", "https://custom.example.com");
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "PAGE TITLE: My Custom Title");
  EXPECT_CONTAINS(content, "PAGE URL: https://custom.example.com");
  EXPECT_CONTAINS(content, "PAGE ROOT DOCUMENT IDENTIFIER: main_doc");
}

TEST_F(PageContentBlocksTest, ExtractViewportGeometry) {
  auto page = CreatePageWithViewport(800, 600, 100, 200);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content,
                  "VIEWPORT: 800x600 pixels, currently scrolled at 100,200");
}

TEST_F(PageContentBlocksTest, EscapeSpecialCharactersInMetadata) {
  auto page =
      CreateMinimalPage("Title with <special> & \"quoted\" content",
                        "https://example.com/path?param=value&other=\"test\"");
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content,
                  "PAGE TITLE: Title with &lt;special&gt; &amp; "
                  "&quot;quoted&quot; content");
  EXPECT_CONTAINS(
      content,
      "PAGE URL: "
      "https://example.com/path?param=value&amp;other=&quot;test&quot;");
}

// ===== CONTENT NODE TYPE CONVERSION TESTS =====

TEST_F(PageContentBlocksTest, ConvertTextNode) {
  auto text_node = CreateTextNode("Simple text content");
  auto page = CreatePageWithContent(text_node);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<text>Simple text content</text>");
}

TEST_F(PageContentBlocksTest, ConvertHeadingNode) {
  auto heading = CreateHeadingNode("Main Heading");
  auto page = CreatePageWithContent(heading);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<heading>Main Heading</heading>");
}

TEST_F(PageContentBlocksTest, ConvertParagraphNode) {
  auto text1 = CreateTextNode("First sentence.");
  auto text2 = CreateTextNode("Second sentence.");
  auto paragraph = CreateParagraphNode({text1, text2});
  auto page = CreatePageWithContent(paragraph);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<paragraph>");
  EXPECT_CONTAINS(content, "<text>First sentence.</text>");
  EXPECT_CONTAINS(content, "<text>Second sentence.</text>");
  EXPECT_CONTAINS(content, "</paragraph>");
}

TEST_F(PageContentBlocksTest, ConvertAnchorNode) {
  auto anchor = CreateAnchorNode("https://example.com", "Click here");
  auto page = CreatePageWithContent(anchor);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content,
                  "<link href=\"https://example.com\">Click here</link>");
}

TEST_F(PageContentBlocksTest, ConvertImageNode) {
  auto image = CreateImageNode("Alt text for image");
  auto page = CreatePageWithContent(image);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<image alt=\"Alt text for image\" />");
}

TEST_F(PageContentBlocksTest, ConvertFormNode) {
  auto input1 = CreateFormControlNode("email", "", "Enter email");
  auto input2 = CreateFormControlNode("password", "", "Enter password");
  auto form = CreateFormNode("loginform", {input1, input2});
  auto page = CreatePageWithContent(form);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<form name=\"loginform\">");
  EXPECT_CONTAINS(content,
                  "<input name=\"email\" placeholder=\"Enter email\" />");
  EXPECT_CONTAINS(content,
                  "<input name=\"password\" placeholder=\"Enter password\" />");
  EXPECT_CONTAINS(content, "</form>");
}

TEST_F(PageContentBlocksTest, ConvertFormControlNodeWithValue) {
  auto input = CreateFormControlNode("username", "john_doe", "Username");
  auto page = CreatePageWithContent(input);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content,
                  "<input name=\"username\" value=\"john_doe\" "
                  "placeholder=\"Username\" />");
}

TEST_F(PageContentBlocksTest, ConvertTableStructure) {
  auto cell1 = CreateTableCellNode({CreateTextNode("Header 1")});
  auto cell2 = CreateTableCellNode({CreateTextNode("Header 2")});
  auto row = CreateTableRowNode({cell1, cell2});
  auto table = CreateTableNode("data_table", {row});
  auto page = CreatePageWithContent(table);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<table name=\"data_table\">");
  EXPECT_CONTAINS(content, "<tr>");
  EXPECT_CONTAINS(content, "<td>");
  EXPECT_CONTAINS(content, "<text>Header 1</text>");
  EXPECT_CONTAINS(content, "<text>Header 2</text>");
  EXPECT_CONTAINS(content, "</td>");
  EXPECT_CONTAINS(content, "</tr>");
  EXPECT_CONTAINS(content, "</table>");
}

TEST_F(PageContentBlocksTest, ConvertOrderedListStructure) {
  auto item1 = CreateListItemNode({CreateTextNode("First item")});
  auto item2 = CreateListItemNode({CreateTextNode("Second item")});
  auto list = CreateListNode(true, {item1, item2});  // ordered = true
  auto page = CreatePageWithContent(list);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<ol>");
  EXPECT_CONTAINS(content, "<li>");
  EXPECT_CONTAINS(content, "<text>First item</text>");
  EXPECT_CONTAINS(content, "<text>Second item</text>");
  EXPECT_CONTAINS(content, "</li>");
  EXPECT_CONTAINS(content, "</ol>");
}

TEST_F(PageContentBlocksTest, ConvertUnorderedListStructure) {
  auto item1 = CreateListItemNode({CreateTextNode("Bullet item")});
  auto list = CreateListNode(false, {item1});  // ordered = false
  auto page = CreatePageWithContent(list);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<ul>");
  EXPECT_CONTAINS(content, "<li>");
  EXPECT_CONTAINS(content, "<text>Bullet item</text>");
  EXPECT_CONTAINS(content, "</li>");
  EXPECT_CONTAINS(content, "</ul>");
}

TEST_F(PageContentBlocksTest, ConvertContainerNode) {
  auto text = CreateTextNode("Container content");
  auto container = CreateContainerNode({text});
  auto page = CreatePageWithContent(container);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<container>");
  EXPECT_CONTAINS(content, "<text>Container content</text>");
  EXPECT_CONTAINS(content, "</container>");
}

TEST_F(PageContentBlocksTest, ConvertIframeNode) {
  auto iframe_content = CreateTextNode("Iframe content");
  auto iframe = CreateIframeNode("iframe_doc_123", {iframe_content});
  auto page = CreatePageWithContent(iframe);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<iframe document_identifier=\"iframe_doc_123\">");
  EXPECT_CONTAINS(content, "<text>Iframe content</text>");
  EXPECT_CONTAINS(content, "</iframe>");
}

TEST_F(PageContentBlocksTest, ConvertSvgNodeWithContent) {
  auto svg = CreateSvgNode("SVG inner text");
  auto page = CreatePageWithContent(svg);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<svg>SVG inner text</svg>");
}

TEST_F(PageContentBlocksTest, ConvertSvgNodeEmpty) {
  auto svg = CreateSvgNode();
  auto page = CreatePageWithContent(svg);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<svg />");
}

TEST_F(PageContentBlocksTest, ConvertVideoNode) {
  auto video = CreateVideoNode("https://example.com/video.mp4");
  auto page = CreatePageWithContent(video);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<video src=\"https://example.com/video.mp4\" />");
}

TEST_F(PageContentBlocksTest, ConvertCanvasNode) {
  auto canvas = CreateCanvasNode();
  auto page = CreatePageWithContent(canvas);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<canvas />");
}

TEST_F(PageContentBlocksTest, ConvertEmptyTextNode) {
  auto empty_text = CreateTextNode("");
  auto page = CreatePageWithContent(empty_text);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  // Empty text nodes should not appear in output
  EXPECT_NOT_CONTAINS(content, "<text></text>");
  EXPECT_NOT_CONTAINS(content, "<text>");
}

TEST_F(PageContentBlocksTest, ConvertWhitespaceOnlyTextNode) {
  auto whitespace_text = CreateTextNode("   \n\t   ");
  auto page = CreatePageWithContent(whitespace_text);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  // Whitespace-only text nodes should not appear in output
  EXPECT_NOT_CONTAINS(content, "<text>");
}

// ===== INTERACTION ATTRIBUTES TESTS =====

TEST_F(PageContentBlocksTest, DetectClickableElement) {
  auto button = CreateClickableButton("Submit", 123, 10, 20);
  auto page = CreatePageWithContent(button);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "clickable");
  EXPECT_CONTAINS(content, "dom_id=\"123\"");
  EXPECT_CONTAINS(content, "x=\"10\" y=\"20\"");
}

TEST_F(PageContentBlocksTest, DetectEditableElement) {
  auto input = CreateEditableInput("email", "Enter email", 456);
  auto page = CreatePageWithContent(input);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "editable");
  EXPECT_CONTAINS(content, "dom_id=\"456\"");
}

TEST_F(PageContentBlocksTest, DetectNonInteractiveElement) {
  auto text = CreateTextNode("Plain text");
  auto page = CreatePageWithContent(text);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_NOT_CONTAINS(content, "clickable");
  EXPECT_NOT_CONTAINS(content, "editable");
  EXPECT_NOT_CONTAINS(content, "dom_id=");
  EXPECT_NOT_CONTAINS(content, "x=");
  EXPECT_NOT_CONTAINS(content, "y=");
}

TEST_F(PageContentBlocksTest, IncludeGeometryForInteractiveElements) {
  auto button = CreateTextNode("Button");
  MakeClickable(button, 789);
  AddGeometry(button, 50, 100, 150, 40);
  auto page = CreatePageWithContent(button);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "dom_id=\"789\"");
  EXPECT_CONTAINS(content, "x=\"50\" y=\"100\" width=\"150\" height=\"40\"");
}

TEST_F(PageContentBlocksTest, ExcludeGeometryForNonInteractiveElements) {
  auto text = CreateTextNode("Regular text");
  AddGeometry(text, 10, 20, 100, 30);  // Geometry added but shouldn't show
  auto page = CreatePageWithContent(text);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_NOT_CONTAINS(content, "x=");
  EXPECT_NOT_CONTAINS(content, "y=");
  EXPECT_NOT_CONTAINS(content, "width=");
  EXPECT_NOT_CONTAINS(content, "height=");
}

TEST_F(PageContentBlocksTest, ConvertImportantRoles) {
  auto header = CreateHeadingNode("Main Header");
  AddRole(header, optimization_guide::proto::ANNOTATED_ROLE_HEADER);
  AddRole(header, optimization_guide::proto::ANNOTATED_ROLE_MAIN);
  auto page = CreatePageWithContent(header);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "role=\"header main\"");
}

TEST_F(PageContentBlocksTest, IncludeAccessibilityLabel) {
  auto button = CreateTextNode("Click");
  AddLabel(button, "Submit button for form");
  auto page = CreatePageWithContent(button);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "label=\"Submit button for form\"");
}

// ===== XML ESCAPING TESTS =====

TEST_F(PageContentBlocksTest, EscapeXmlSpecialCharactersInText) {
  auto text = CreateTextNode("Text with <tags> & \"quotes\" and 'apostrophes'");
  auto page = CreatePageWithContent(text);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content,
                  "Text with &lt;tags&gt; &amp; &quot;quotes&quot; and "
                  "&apos;apostrophes&apos;");
}

TEST_F(PageContentBlocksTest, EscapeXmlSpecialCharactersInAttributes) {
  auto anchor =
      CreateAnchorNode("https://example.com?param=value&other=\"test\"",
                       "Link with <special> chars");
  auto page = CreatePageWithContent(anchor);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(
      content,
      "href=\"https://example.com?param=value&amp;other=&quot;test&quot;\"");
  EXPECT_CONTAINS(content, "Link with &lt;special&gt; chars");
}

TEST_F(PageContentBlocksTest, EscapeAccessibilityLabel) {
  auto button = CreateTextNode("Button");
  AddLabel(button, "Label with <special> & \"quoted\" content");
  auto page = CreatePageWithContent(button);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(
      content,
      "label=\"Label with &lt;special&gt; &amp; &quot;quoted&quot; content\"");
}

// ===== CONTAINER FLATTENING TESTS =====

TEST_F(PageContentBlocksTest, FlattenSingleChildNonInteractiveContainer) {
  auto inner_text = CreateTextNode("Inner content");
  auto container = CreateContainerNode({inner_text});
  auto page = CreatePageWithContent(container);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  // Container should be flattened, only the text should appear
  EXPECT_CONTAINS(content, "<text>Inner content</text>");
  EXPECT_NOT_CONTAINS(content, "<container>");
}

TEST_F(PageContentBlocksTest, PreserveInteractiveContainer) {
  auto inner_text = CreateTextNode("Inner content");
  auto container = CreateContainerNode({inner_text});
  MakeClickable(container, 999);
  auto page = CreatePageWithContent(container);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  // Interactive container should NOT be flattened
  EXPECT_CONTAINS(content, "<container");
  EXPECT_CONTAINS(content, "clickable");
  EXPECT_CONTAINS(content, "dom_id=\"999\"");
  EXPECT_CONTAINS(content, "<text>Inner content</text>");
  EXPECT_CONTAINS(content, "</container>");
}

TEST_F(PageContentBlocksTest, PreserveMultiChildContainer) {
  auto text1 = CreateTextNode("First");
  auto text2 = CreateTextNode("Second");
  auto container = CreateContainerNode({text1, text2});
  auto page = CreatePageWithContent(container);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  // Multi-child container should NOT be flattened
  EXPECT_CONTAINS(content, "<container>");
  EXPECT_CONTAINS(content, "<text>First</text>");
  EXPECT_CONTAINS(content, "<text>Second</text>");
  EXPECT_CONTAINS(content, "</container>");
}

TEST_F(PageContentBlocksTest, PreserveContainerWithText) {
  auto container = ContentNodeBuilder()
                       .AsContainer()
                       .WithText("Container text")
                       .WithChildren({CreateTextNode("Child text")})
                       .Build();
  auto page = CreatePageWithContent(container);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  // Container with its own text should NOT be flattened
  EXPECT_CONTAINS(content, "<container>");
}

// ===== COMPLEX INTEGRATION TESTS =====

TEST_F(PageContentBlocksTest, ConvertComplexPageStructure) {
  auto page = CreatePageWithComplexStructure();
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "PAGE TITLE: Complex Page");
  EXPECT_CONTAINS(content, "<heading role=\"header\">Welcome</heading>");
  EXPECT_CONTAINS(content, "role=\"nav\"");
  EXPECT_CONTAINS(content, "role=\"main\"");
  EXPECT_CONTAINS(content, "Navigation");
  EXPECT_CONTAINS(content, "Main content goes here");
}

TEST_F(PageContentBlocksTest, ConvertFormWithMultipleInputs) {
  auto page = CreatePageWithFormElements();
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<form name=\"loginform\">");
  EXPECT_CONTAINS(content,
                  "<input name=\"email\" placeholder=\"Enter email\" "
                  "dom_id=\"101\" editable />");
  EXPECT_CONTAINS(content,
                  "<input name=\"password\" placeholder=\"Enter password\" "
                  "dom_id=\"102\" editable />");
  EXPECT_CONTAINS(content, "dom_id=\"103\" clickable");
  EXPECT_CONTAINS(content, "</form>");
}

TEST_F(PageContentBlocksTest, ConvertTableWithHeadersAndData) {
  auto page = CreatePageWithTableStructure();
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<table name=\"data_table\">");
  EXPECT_CONTAINS(content, "<tr>");
  EXPECT_CONTAINS(content, "<td>");
  EXPECT_CONTAINS(content, "Header 1");
  EXPECT_CONTAINS(content, "Header 2");
  EXPECT_CONTAINS(content, "Data 1");
  EXPECT_CONTAINS(content, "Data 2");
  EXPECT_CONTAINS(content, "</td>");
  EXPECT_CONTAINS(content, "</tr>");
  EXPECT_CONTAINS(content, "</table>");
}

TEST_F(PageContentBlocksTest, ConvertPageWithInteractiveElements) {
  auto page = CreatePageWithInteractiveElements();
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "dom_id=\"201\" x=\"10\" y=\"10\"");
  EXPECT_CONTAINS(
      content, "dom_id=\"202\" x=\"10\" y=\"50\" width=\"300\" height=\"25\"");
  EXPECT_CONTAINS(
      content, "dom_id=\"203\" x=\"10\" y=\"100\" width=\"100\" height=\"20\"");
  EXPECT_CONTAINS(content, "clickable");
  EXPECT_CONTAINS(content, "editable");
}

// ===== OUTPUT STRUCTURE TESTS =====

TEST_F(PageContentBlocksTest, VerifyCompleteOutputStructure) {
  auto page = CreateMinimalPage();
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  // Verify sections appear in correct order
  auto metadata_pos = content.find("=== PAGE METADATA ===");
  auto structure_pos =
      content.find("=== PAGE STRUCTURE (XML representation) ===");
  auto instructions_pos = content.find("=== INTERACTION INSTRUCTIONS ===");

  EXPECT_NE(metadata_pos, std::string::npos);
  EXPECT_NE(structure_pos, std::string::npos);
  EXPECT_NE(instructions_pos, std::string::npos);

  EXPECT_LT(metadata_pos, structure_pos);
  EXPECT_LT(structure_pos, instructions_pos);
}

TEST_F(PageContentBlocksTest, VerifyInteractionInstructions) {
  auto page = CreateMinimalPage();
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "dom_id: Use for precise element targeting");
  EXPECT_CONTAINS(content, "x,y,width,height: Use the position/size");
  EXPECT_CONTAINS(content, "clickable: Element can be clicked");
  EXPECT_CONTAINS(content, "editable: Element can receive text input");
}

TEST_F(PageContentBlocksTest, VerifyContentBlockCreation) {
  auto page = CreateMinimalPage();
  auto result = ConvertAnnotatedPageContentToBlocks(page);

  // Should return exactly one ContentBlock
  EXPECT_EQ(result.size(), 1u);
  EXPECT_TRUE(result[0]->is_text_content_block());
  EXPECT_FALSE(result[0]->get_text_content_block()->text.empty());
}

// ===== SIZE LIMITS AND PERFORMANCE TESTS =====

TEST_F(PageContentBlocksTest, HandleLargeContent) {
  // Create a page with many elements to test size limits
  std::vector<optimization_guide::proto::ContentNode> many_elements;
  for (int i = 0; i < 1000; ++i) {
    many_elements.push_back(
        CreateTextNode("This is element number " + base::NumberToString(i) +
                       " with lots of repeated content to make it long"));
  }

  auto container = CreateContainerNode(std::move(many_elements));
  auto page = CreatePageWithContent(container);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  // Should be truncated with proper message
  EXPECT_CONTAINS(content,
                  "PAGE STRUCTURE (XML) was too long to display. Truncated.");
  EXPECT_CONTAINS(content, "...</root>");
}

TEST_F(PageContentBlocksTest, HandleDeeplyNestedStructure) {
  // Create a deeply nested structure
  auto inner_text = CreateTextNode("Deep content");
  optimization_guide::proto::ContentNode current = inner_text;

  // Create 50 levels of nesting
  for (int i = 0; i < 50; ++i) {
    current = CreateContainerNode({current});
  }

  auto page = CreatePageWithContent(current);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  // Should handle deep nesting without crashing
  EXPECT_EQ(result.size(), 1u);
  EXPECT_CONTAINS(content, "Deep content");
}

// ===== BUILDER PATTERN TESTS =====

TEST_F(PageContentBlocksTest, UseBuilderForComplexElement) {
  auto complex_element =
      ContentNodeBuilder()
          .AsAnchor("https://example.com", "Complex Link")
          .MakeClickable(987)
          .WithGeometry(25, 75, 200, 35)
          .WithRole(optimization_guide::proto::ANNOTATED_ROLE_NAV)
          .WithLabel("Navigation link")
          .Build();

  auto page = CreatePageWithContent(complex_element);
  auto result = ConvertAnnotatedPageContentToBlocks(page);
  auto content = GetContentText(result);

  EXPECT_CONTAINS(content, "<link href=\"https://example.com\"");
  EXPECT_CONTAINS(content, "dom_id=\"987\"");
  EXPECT_CONTAINS(content, "x=\"25\" y=\"75\" width=\"200\" height=\"35\"");
  EXPECT_CONTAINS(content, "clickable");
  EXPECT_CONTAINS(content, "role=\"nav\"");
  EXPECT_CONTAINS(content, "label=\"Navigation link\"");
  EXPECT_CONTAINS(content, "Complex Link</link>");
}

}  // namespace ai_chat
