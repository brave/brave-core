// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/page_content_blocks.h"

#include <string>

#include "base/logging.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace ai_chat {

namespace {

using optimization_guide::proto::AnnotatedPageContent;
using optimization_guide::proto::ContentAttributes;
using optimization_guide::proto::ContentAttributeType;
using optimization_guide::proto::ContentNode;

constexpr size_t kMaxTreeStringLength = 100000;

bool ShouldFlattenContainer(const ContentNode& node) {
  // Only consider flattening if there's exactly one child
  if (node.children_nodes_size() != 1) {
    return false;
  }

  const auto& attrs = node.content_attributes();
  const auto& interaction = attrs.interaction_info();
  // Flatten non-interactive containers with only 1 child
  // but don't consider focusable, selectable and draggable for now
  if (interaction.is_clickable() || interaction.is_editable()) {
    return false;
  }

  // Don't flatten scrollable containers
  if (interaction.has_scroller_info()) {
    const auto& scroller_info = interaction.scroller_info();
    if (scroller_info.user_scrollable_horizontal() ||
        scroller_info.user_scrollable_vertical()) {
      return false;
    }
  }

  // Don't flatten if it has text
  if (attrs.has_text_data() && attrs.text_data().has_text_content() &&
      !attrs.text_data().text_content().empty()) {
    return false;
  }

  // Don't flatten if it has any specific content data (anchor, image, form,
  // etc.) These are in a oneof, so check using the case accessor
  if (attrs.content_data_case() != ContentAttributes::CONTENT_DATA_NOT_SET) {
    return false;
  }

  // Don't flatten if it has a role
  if (attrs.annotated_roles_size() > 0) {
    return false;
  }

  // Don't flatten the root node
  if (node.content_attributes().attribute_type() ==
      ContentAttributeType::CONTENT_ATTRIBUTE_ROOT) {
    return false;
  }

  return true;
}

// Remove the untrusted content tag from the input
[[nodiscard]] std::string SanitizeContentText(const std::string& input) {
  std::string output = input;
  // Avoid content breaking out of untrusted tags
  base::ReplaceSubstringsAfterOffset(&output, 0, kBraveUntrustedContentTagName,
                                     "");
  return output;
}

// XML escape and remove the untrusted content tag from the input
[[nodiscard]] std::string XmlEscapeAndSanitizeText(const std::string& input) {
  std::string output = SanitizeContentText(input);

  // Escape XML to avoid breaking out of pseudo-XML serialization
  return base::EscapeForHTML(output);
}

std::string BuildAttributes(const ContentAttributes& attrs,
                            bool id_only_for_interactive = true) {
  // Helper function to build interaction attributes
  std::string attr_result;

  // Check if element is interactive
  bool is_interactive = false;
  if (attrs.has_interaction_info()) {
    const auto& interaction = attrs.interaction_info();
    if (interaction.has_scroller_info()) {
      const auto& scroller_info = interaction.scroller_info();
      if (scroller_info.user_scrollable_horizontal() ||
          scroller_info.user_scrollable_vertical()) {
        is_interactive = true;
      }
    }
    if (!is_interactive &&
        (interaction.is_clickable() || interaction.is_editable())) {
      // ignore selectable, focusable and draggable for now
      is_interactive = true;
    }
  }

  // Add DOM node ID if available
  if ((is_interactive || !id_only_for_interactive) &&
      attrs.has_common_ancestor_dom_node_id()) {
    absl::StrAppendFormat(&attr_result, " dom_id=\"%d\"",
                          attrs.common_ancestor_dom_node_id());
  }

  // Add interaction capabilities
  if (attrs.has_interaction_info()) {
    const auto& interaction = attrs.interaction_info();
    if (interaction.is_clickable()) {
      attr_result.append(" clickable");
    }
    if (interaction.is_editable()) {
      attr_result.append(" editable");
    }
    if (interaction.has_scroller_info()) {
      const auto& scroller_info = interaction.scroller_info();
      if (scroller_info.user_scrollable_horizontal() ||
          scroller_info.user_scrollable_vertical()) {
        auto size = scroller_info.scrolling_bounds();
        auto visible_area = scroller_info.visible_area();
        attr_result.append(" scrollable");
        // Size in XxY
        absl::StrAppendFormat(&attr_result, " size=\"%dx%d\"", size.width(),
                              size.height());
        // Visible area size and position
        absl::StrAppendFormat(&attr_result, " visible_area=\"%dx%d,%d,%d\"",
                              visible_area.width(), visible_area.height(),
                              visible_area.x(), visible_area.y());
      }
    }

    // Add geometry only if interactive
    if (is_interactive && attrs.has_geometry() &&
        attrs.geometry().has_outer_bounding_box()) {
      const auto& bbox = attrs.geometry().outer_bounding_box();
      absl::StrAppendFormat(&attr_result,
                            " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"",
                            bbox.x(), bbox.y(), bbox.width(), bbox.height());
    }
  }

  // Add important roles
  if (attrs.annotated_roles_size() > 0) {
    std::vector<std::string> important_roles;
    for (const auto& role : attrs.annotated_roles()) {
      switch (role) {
        case optimization_guide::proto::ANNOTATED_ROLE_HEADER:
          important_roles.push_back("header");
          break;
        case optimization_guide::proto::ANNOTATED_ROLE_NAV:
          important_roles.push_back("nav");
          break;
        case optimization_guide::proto::ANNOTATED_ROLE_SEARCH:
          important_roles.push_back("search");
          break;
        case optimization_guide::proto::ANNOTATED_ROLE_MAIN:
          important_roles.push_back("main");
          break;
        case optimization_guide::proto::ANNOTATED_ROLE_ARTICLE:
          important_roles.push_back("article");
          break;
        case optimization_guide::proto::ANNOTATED_ROLE_SECTION:
          important_roles.push_back("section");
          break;
        case optimization_guide::proto::ANNOTATED_ROLE_ASIDE:
          important_roles.push_back("aside");
          break;
        case optimization_guide::proto::ANNOTATED_ROLE_FOOTER:
          important_roles.push_back("footer");
          break;
        case optimization_guide::proto::ANNOTATED_ROLE_CONTENT_HIDDEN:
          important_roles.push_back("hidden");
          break;
        case optimization_guide::proto::ANNOTATED_ROLE_PAID_CONTENT:
          important_roles.push_back("paid");
          break;
        default:
          // Skip unknown roles
          break;
      }
    }
    if (!important_roles.empty()) {
      base::StrAppend(
          &attr_result,
          {" role=\"",
           XmlEscapeAndSanitizeText(base::JoinString(important_roles, " ")),
           "\""});
    }
  }

  if (attrs.has_iframe_data() && attrs.iframe_data().has_frame_data() &&
      attrs.iframe_data().frame_data().has_document_identifier()) {
    base::StrAppend(&attr_result,
                    {" document_identifier=\"",
                     XmlEscapeAndSanitizeText(attrs.iframe_data()
                                                  .frame_data()
                                                  .document_identifier()
                                                  .serialized_token()),
                     "\""});
  }

  // Add accessibility label if available
  if (attrs.has_label() && !attrs.label().empty()) {
    base::StrAppend(
        &attr_result,
        {" label=\"", XmlEscapeAndSanitizeText(attrs.label()), "\""});
  }

  return attr_result;
}

// Generates XML-like structured content representation with interaction
// attributes
std::string GenerateContentStructure(const ContentNode& node, int depth = 0) {
  std::string content;
  std::string indent(
      features::kShouldIndentPageContentBlocks.Get() ? depth * 2 : 0, ' ');

  const auto& attrs = node.content_attributes();

  // Flatten single-child root containers
  if (ShouldFlattenContainer(node)) {
    CHECK_EQ(node.children_nodes_size(), 1);
    return GenerateContentStructure(node.children_nodes(0), depth);
  }

  // Generate the tag name, initial attributes and intrinsic "child" content.
  // Actual children elements will be handled after the switch unless a case
  // has custom handling in which case it should return early.
  std::string tag_name = "";
  std::string inner_content = "";
  std::string attributes = "";
  switch (attrs.attribute_type()) {
    case ContentAttributeType::CONTENT_ATTRIBUTE_HEADING:
      if (attrs.has_text_data()) {
        tag_name = "heading";
        attributes = BuildAttributes(attrs);
        inner_content =
            XmlEscapeAndSanitizeText(attrs.text_data().text_content());
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_PARAGRAPH:
      tag_name = "paragraph";
      attributes = BuildAttributes(attrs);
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_TEXT:
      if (attrs.has_text_data()) {
        const std::string& text = attrs.text_data().text_content();
        if (text.empty()) {
          break;
        }
        std::string trimmed;
        base::TrimWhitespaceASCII(text, base::TRIM_ALL, &trimmed);
        if (!trimmed.empty()) {
          // NOTE: For space saving, we could consider flattening text nodes
          // to their parents, since they shouldn't be targetable.
          tag_name = "text";
          attributes = BuildAttributes(attrs);
          inner_content = XmlEscapeAndSanitizeText(trimmed);
        }
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_ANCHOR:
      if (attrs.has_anchor_data()) {
        tag_name = "link";
        base::StrAppend(
            &attributes,
            {"href=\"", XmlEscapeAndSanitizeText(attrs.anchor_data().url()),
             "\""});
        base::StrAppend(&attributes, {BuildAttributes(attrs)});
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_FORM:
      tag_name = "form";
      if (attrs.has_form_data() && attrs.form_data().has_form_name()) {
        base::StrAppend(
            &attributes,
            {"name=\"", XmlEscapeAndSanitizeText(attrs.form_data().form_name()),
             "\""});
      }
      base::StrAppend(&attributes, {BuildAttributes(attrs)});
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_FORM_CONTROL:
      if (attrs.has_form_control_data()) {
        tag_name = "input";
        const auto& form_data = attrs.form_control_data();
        if (form_data.has_field_name()) {
          base::StrAppend(
              &attributes,
              {" name=\"", XmlEscapeAndSanitizeText(form_data.field_name()),
               "\""});
        }
        if (form_data.has_field_value() && !form_data.field_value().empty()) {
          base::StrAppend(
              &attributes,
              {" value=\"", XmlEscapeAndSanitizeText(form_data.field_value()),
               "\""});
        }
        if (form_data.has_placeholder() && !form_data.placeholder().empty()) {
          base::StrAppend(
              &attributes,
              {" placeholder=\"",
               XmlEscapeAndSanitizeText(form_data.placeholder()), "\""});
        }
        base::StrAppend(&attributes, {BuildAttributes(attrs)});
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_IMAGE:
      if (attrs.has_image_data() && attrs.image_data().has_image_caption()) {
        tag_name = "image";
        base::StrAppend(
            &attributes,
            {" alt=\"",
             XmlEscapeAndSanitizeText(attrs.image_data().image_caption()),
             "\""});
        base::StrAppend(&attributes, {BuildAttributes(attrs)});
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_TABLE:
      tag_name = "table";
      if (attrs.has_table_data() && attrs.table_data().has_table_name()) {
        base::StrAppend(
            &attributes,
            {" name=\"",
             XmlEscapeAndSanitizeText(attrs.table_data().table_name()), "\""});
      }
      base::StrAppend(&attributes, {BuildAttributes(attrs)});
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_TABLE_ROW:
      tag_name = "tr";
      attributes = BuildAttributes(attrs);
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_TABLE_CELL:
      tag_name = "td";
      attributes = BuildAttributes(attrs);
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_ORDERED_LIST:
      tag_name = "ol";
      attributes = BuildAttributes(attrs);
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_UNORDERED_LIST:
      tag_name = "ul";
      attributes = BuildAttributes(attrs);
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_LIST_ITEM:
      tag_name = "li";
      attributes = BuildAttributes(attrs);
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_ROOT:
      tag_name = "root";
      attributes = BuildAttributes(attrs, false);
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_CONTAINER:
      tag_name = "container";
      attributes = BuildAttributes(attrs);
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_IFRAME:
      tag_name = "iframe";
      attributes = BuildAttributes(attrs);
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_SVG:
      tag_name = "svg";
      attributes = BuildAttributes(attrs);
      if (attrs.has_svg_data() && attrs.svg_data().has_inner_text()) {
        inner_content = XmlEscapeAndSanitizeText(attrs.svg_data().inner_text());
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_CANVAS:
      tag_name = "canvas";
      attributes = BuildAttributes(attrs);
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_VIDEO:
      tag_name = "video";
      if (attrs.has_video_data() && attrs.video_data().has_url()) {
        base::StrAppend(
            &attributes,
            {" src=\"", XmlEscapeAndSanitizeText(attrs.video_data().url()),
             "\""});
      }
      base::StrAppend(&attributes, {BuildAttributes(attrs)});
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_UNKNOWN:
    case ContentAttributeType::
        ContentAttributeType_INT_MIN_SENTINEL_DO_NOT_USE_:
    case ContentAttributeType::
        ContentAttributeType_INT_MAX_SENTINEL_DO_NOT_USE_:
      // Skip unknown or sentinel values
      break;
  }

  if (!tag_name.empty()) {
    base::StrAppend(&content, {"\n", indent, "<", tag_name});
    if (!attributes.empty()) {
      base::StrAppend(&content, {" ", attributes});
    }
    if (inner_content.empty() && node.children_nodes_size() == 0) {
      content.append(" />");
      return content;
    }
    content.append(">");
    if (!inner_content.empty()) {
      // Add 1x extra depth to indent for inner content
      base::StrAppend(&content, {"\n", indent, "  ", inner_content});
    }
  }

  // Process children for elements that don't handle them explicitly above,
  // adding 1x extra depth.
  for (const auto& child : node.children_nodes()) {
    base::StrAppend(&content, {GenerateContentStructure(child, depth + 1)});
  }

  // Closing tag if we're not flattening or ignoring this element
  if (!tag_name.empty()) {
    base::StrAppend(&content, {"\n", indent, "</", tag_name, ">"});
  }

  return content;
}

}  // namespace

std::vector<mojom::ContentBlockPtr> ConvertAnnotatedPageContentToBlocks(
    const AnnotatedPageContent& page_content) {
  if (!page_content.has_root_node()) {
    return {};
  }

  // Indicate that the content is external and is untrusted
  std::string result = base::StrCat(
      {kBraveUntrustedContentOpenTag, "\n=== PAGE METADATA ===\n\n"});

  // Add page metadata
  if (page_content.has_main_frame_data()) {
    const auto& frame_data = page_content.main_frame_data();
    if (frame_data.has_title()) {
      base::StrAppend(
          &result,
          {"PAGE TITLE: ", SanitizeContentText(frame_data.title()), "\n"});
    }
    if (frame_data.has_url()) {
      base::StrAppend(
          &result, {"PAGE URL: ", SanitizeContentText(frame_data.url()), "\n"});
    }

    if (frame_data.has_document_identifier()) {
      base::StrAppend(
          &result, {"PAGE ROOT DOCUMENT IDENTIFIER: ",
                    frame_data.document_identifier().serialized_token(), "\n"});
    }
    result.append("\n");
  }

  const auto& root_node = page_content.root_node();

  // Add viewport information for coordinate references
  if (page_content.has_viewport_geometry()) {
    const auto& viewport = page_content.viewport_geometry();

    absl::StrAppendFormat(
        &result, "VIEWPORT: %dx%d pixels, currently scrolled at %d,%d",
        viewport.width(), viewport.height(), viewport.x(), viewport.y());

    if (root_node.content_attributes().has_interaction_info() &&
        root_node.content_attributes().interaction_info().has_scroller_info()) {
      const auto& scroller_info =
          root_node.content_attributes().interaction_info().scroller_info();
      absl::StrAppendFormat(&result, " within a document of size %dx%d",
                            scroller_info.scrolling_bounds().width(),
                            scroller_info.scrolling_bounds().height());
    }

    result.append("\n");
  }

  std::string tree_string = GenerateContentStructure(root_node);
  if (tree_string.length() > kMaxTreeStringLength) {
    // TODO(https://github.com/brave/brave-browser/issues/49262): prioritize
    // viewport elements - the consumer can then scroll to "paginate."
    tree_string = tree_string.substr(0, kMaxTreeStringLength) + "...</root>\n";
    tree_string.append(
        "PAGE STRUCTURE (XML) was too long to display. Truncated.\n\n");
  }

  result.append("\n=== PAGE STRUCTURE (XML representation) ===");

  // Replace all occurances of the untrusted tag with an empty string
  tree_string = SanitizeContentText(tree_string);

  base::StrAppend(&result, {tree_string});
  base::StrAppend(&result, {"\n", kBraveUntrustedContentCloseTag, "\n"});

  // Add usage instructions
  result.append("\n=== INTERACTION INSTRUCTIONS ===\n");
  result.append(
      "The page structure represents the entire page and not just the "
      "viewport. Use scroll if neccessary to interact with an element not "
      "within the viewport, or to show the user something. "
      "Use the XML attributes to guide interaction:\n");
  result.append(
      "- dom_id: Use for precise element targeting but you must provide the "
      "document_identifier either from the root or from an iframe.\n");
  result.append(
      "- x,y,width,height: Use the position/size only when neccessary or to "
      "infer hierarchy.\n");
  result.append("- clickable: Element can be clicked\n");
  result.append("- editable: Element can receive text input\n");

  // Convert to ContentBlocks using the existing utility
  return CreateContentBlocksForText(result);
}

}  // namespace ai_chat
