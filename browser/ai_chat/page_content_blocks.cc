// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/page_content_blocks.h"

#include <string>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace ai_chat {

namespace {

using optimization_guide::proto::AnnotatedPageContent;
using optimization_guide::proto::ContentAttributes;
using optimization_guide::proto::ContentAttributeType;
using optimization_guide::proto::ContentNode;

constexpr bool kShouldIncludeGeometry = true;
constexpr bool kShouldIndent = false;
constexpr size_t kMaxTreeStringLength = 100000;

bool ShouldFlattenContainer(const ContentNode& node) {
  // Only consider flattening if there's exactly one child
  if (node.children_nodes_size() > 1) {
    return false;
  }

  const auto& attrs = node.content_attributes();
  const auto& interaction = attrs.interaction_info();
  // Flatten non-interactive containers with only 1 child
  // but don't consider focusable, selectable and draggable for now
  if (interaction.is_clickable() || interaction.is_editable()) {
    return false;
  }

  // Don't flatten if it has text
  if (attrs.has_text_data() && attrs.text_data().has_text_content() &&
      !attrs.text_data().text_content().empty()) {
    return false;
  }

  return true;
}

// XML escape functions
std::string XmlEscapeAndSanitizeText(const std::string& input) {
  std::string output = input;
  // Avoid content breaking out of untrusted tags
  base::ReplaceSubstringsAfterOffset(&output, 0, kBraveUntrustedContentTagName,
                                     "");

  // Escape XML to avoid breaking out of pseudo-XML serialization
  base::ReplaceChars(output, "&", "&amp;", &output);
  base::ReplaceChars(output, "<", "&lt;", &output);
  base::ReplaceChars(output, ">", "&gt;", &output);
  base::ReplaceChars(output, "\"", "&quot;", &output);
  base::ReplaceChars(output, "'", "&apos;", &output);
  return output;
}

std::string BuildAttributes(const ContentAttributes& attrs) {
  // Helper function to build interaction attributes
  std::string attr_result;

  // Check if element is interactive
  bool is_interactive = false;
  if (attrs.has_interaction_info()) {
    const auto& interaction = attrs.interaction_info();
    if (interaction.is_clickable() || interaction.is_editable()) {
      // ignore selectable, focusable and draggable for now
      is_interactive = true;
    }
  }

  // Add DOM node ID if available
  if (is_interactive && attrs.has_common_ancestor_dom_node_id()) {
    attr_result +=
        absl::StrFormat(" dom_id=\"%d\"", attrs.common_ancestor_dom_node_id());
  }

  // Add geometry only if interactive
  if (kShouldIncludeGeometry && is_interactive && attrs.has_geometry() &&
      attrs.geometry().has_outer_bounding_box()) {
    const auto& bbox = attrs.geometry().outer_bounding_box();
    attr_result +=
        absl::StrFormat(" x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"",
                        bbox.x(), bbox.y(), bbox.width(), bbox.height());
  }

  // Add interaction capabilities
  if (attrs.has_interaction_info()) {
    const auto& interaction = attrs.interaction_info();
    if (interaction.is_clickable()) {
      attr_result += " clickable";
    }
    if (interaction.is_editable()) {
      attr_result += " editable";
    }
    // if (interaction.is_focusable()) {
    //   attr_result += " focusable=\"true\"";
    // }
    // if (interaction.is_selectable()) {
    //   attr_result += " selectable=\"true\"";
    // }
    // if (interaction.is_draggable()) {
    //   attr_result += " draggable=\"true\"";
    // }
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
      attr_result += base::StrCat(
          {" role=\"",
           XmlEscapeAndSanitizeText(base::JoinString(important_roles, " ")),
           "\""});
    }
  }

  if (attrs.has_iframe_data() && attrs.iframe_data().has_frame_data() &&
      attrs.iframe_data().frame_data().has_document_identifier()) {
    attr_result +=
        base::StrCat({" document_identifier=\"",
                      XmlEscapeAndSanitizeText(attrs.iframe_data()
                                                   .frame_data()
                                                   .document_identifier()
                                                   .serialized_token()),
                      "\""});
  }

  // Add accessibility label if available
  if (attrs.has_label() && !attrs.label().empty()) {
    attr_result += base::StrCat(
        {" label=\"", XmlEscapeAndSanitizeText(attrs.label()), "\""});
  }

  return attr_result;
}

// Generates XML-like structured content representation with interaction
// attributes
std::string GenerateContentStructure(const ContentNode& node, int depth = 0) {
  std::string content;
  std::string indent(kShouldIndent ? depth * 2 : 0, ' ');

  const auto& attrs = node.content_attributes();

  // Flatten single-child root containers
  if (ShouldFlattenContainer(node)) {
    if (node.children_nodes_size() == 1) {
      return GenerateContentStructure(node.children_nodes(0), depth);
    }
    return "";
  }

  switch (attrs.attribute_type()) {
    case ContentAttributeType::CONTENT_ATTRIBUTE_HEADING:
      if (attrs.has_text_data()) {
        content += base::StrCat(
            {indent, "<heading", BuildAttributes(attrs), ">",
             XmlEscapeAndSanitizeText(attrs.text_data().text_content()),
             "</heading>\n"});
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_PARAGRAPH:
      content +=
          base::StrCat({indent, "<paragraph", BuildAttributes(attrs), ">\n"});
      for (const auto& child : node.children_nodes()) {
        content += GenerateContentStructure(child, depth + 1);
      }
      content += base::StrCat({indent, "</paragraph>\n"});
      return content;

    case ContentAttributeType::CONTENT_ATTRIBUTE_TEXT:
      if (attrs.has_text_data()) {
        const std::string& text = attrs.text_data().text_content();
        if (text.empty()) {
          break;
        }
        std::string trimmed;
        base::TrimWhitespaceASCII(text, base::TRIM_ALL, &trimmed);
        if (!trimmed.empty()) {
          content +=
              base::StrCat({indent, "<text", BuildAttributes(attrs), ">",
                            XmlEscapeAndSanitizeText(trimmed), "</text>\n"});
        }
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_ANCHOR:
      if (attrs.has_anchor_data()) {
        content +=
            base::StrCat({indent, "<link href=\"",
                          XmlEscapeAndSanitizeText(attrs.anchor_data().url()),
                          "\"", BuildAttributes(attrs), ">"});
        if (attrs.has_text_data()) {
          content += XmlEscapeAndSanitizeText(attrs.text_data().text_content());
        }
        content += "</link>\n";
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_FORM:
      content += base::StrCat({indent, "<form"});
      if (attrs.has_form_data() && attrs.form_data().has_form_name()) {
        content += base::StrCat(
            {" name=\"",
             XmlEscapeAndSanitizeText(attrs.form_data().form_name()), "\""});
      }
      content += base::StrCat({BuildAttributes(attrs), ">\n"});
      for (const auto& child : node.children_nodes()) {
        content += GenerateContentStructure(child, depth + 1);
      }
      content += base::StrCat({indent, "</form>\n"});
      return content;

    case ContentAttributeType::CONTENT_ATTRIBUTE_FORM_CONTROL:
      if (attrs.has_form_control_data()) {
        const auto& form_data = attrs.form_control_data();
        content += base::StrCat({indent, "<input"});
        if (form_data.has_field_name()) {
          content += base::StrCat(
              {" name=\"", XmlEscapeAndSanitizeText(form_data.field_name()),
               "\""});
        }
        if (form_data.has_field_value() && !form_data.field_value().empty()) {
          content += base::StrCat(
              {" value=\"", XmlEscapeAndSanitizeText(form_data.field_value()),
               "\""});
        }
        if (form_data.has_placeholder() && !form_data.placeholder().empty()) {
          content += base::StrCat(
              {" placeholder=\"",
               XmlEscapeAndSanitizeText(form_data.placeholder()), "\""});
        }
        content += base::StrCat({BuildAttributes(attrs), " />\n"});
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_IMAGE:
      if (attrs.has_image_data() && attrs.image_data().has_image_caption()) {
        content += base::StrCat({indent, "<image"});
        content += base::StrCat(
            {" alt=\"",
             XmlEscapeAndSanitizeText(attrs.image_data().image_caption()),
             "\""});
        content += base::StrCat({BuildAttributes(attrs), " />\n"});
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_TABLE:
      content += base::StrCat({indent, "<table"});
      if (attrs.has_table_data() && attrs.table_data().has_table_name()) {
        content += base::StrCat(
            {" name=\"",
             XmlEscapeAndSanitizeText(attrs.table_data().table_name()), "\""});
      }
      content += base::StrCat({BuildAttributes(attrs), ">\n"});
      for (const auto& child : node.children_nodes()) {
        content += GenerateContentStructure(child, depth + 1);
      }
      content += base::StrCat({indent, "</table>\n"});
      return content;

    case ContentAttributeType::CONTENT_ATTRIBUTE_TABLE_ROW:
      content += base::StrCat({indent, "<tr", BuildAttributes(attrs), ">\n"});
      for (const auto& child : node.children_nodes()) {
        content += GenerateContentStructure(child, depth + 1);
      }
      content += base::StrCat({indent, "</tr>\n"});
      return content;

    case ContentAttributeType::CONTENT_ATTRIBUTE_TABLE_CELL:
      content += base::StrCat({indent, "<td", BuildAttributes(attrs), ">\n"});
      for (const auto& child : node.children_nodes()) {
        content += GenerateContentStructure(child, depth + 1);
      }
      content += base::StrCat({indent, "</td>\n"});
      return content;

    case ContentAttributeType::CONTENT_ATTRIBUTE_ORDERED_LIST:
      content += base::StrCat({indent, "<ol", BuildAttributes(attrs), ">\n"});
      for (const auto& child : node.children_nodes()) {
        content += GenerateContentStructure(child, depth + 1);
      }
      content += base::StrCat({indent, "</ol>\n"});
      return content;

    case ContentAttributeType::CONTENT_ATTRIBUTE_UNORDERED_LIST:
      content += base::StrCat({indent, "<ul", BuildAttributes(attrs), ">\n"});
      for (const auto& child : node.children_nodes()) {
        content += GenerateContentStructure(child, depth + 1);
      }
      content += base::StrCat({indent, "</ul>\n"});
      return content;

    case ContentAttributeType::CONTENT_ATTRIBUTE_LIST_ITEM:
      content += base::StrCat({indent, "<li", BuildAttributes(attrs), ">\n"});
      for (const auto& child : node.children_nodes()) {
        content += GenerateContentStructure(child, depth + 1);
      }
      content += base::StrCat({indent, "</li>\n"});
      return content;

    case ContentAttributeType::CONTENT_ATTRIBUTE_ROOT:
      content += base::StrCat({indent, "<page", BuildAttributes(attrs), ">\n"});
      for (const auto& child : node.children_nodes()) {
        content += GenerateContentStructure(child, depth + 1);
      }
      content += base::StrCat({indent, "</page>\n"});
      return content;

    case ContentAttributeType::CONTENT_ATTRIBUTE_CONTAINER:
      content +=
          base::StrCat({indent, "<container", BuildAttributes(attrs), ">\n"});
      for (const auto& child : node.children_nodes()) {
        content += GenerateContentStructure(child, depth + 1);
      }
      content += base::StrCat({indent, "</container>\n"});
      return content;

    case ContentAttributeType::CONTENT_ATTRIBUTE_IFRAME:
      content +=
          base::StrCat({indent, "<iframe", BuildAttributes(attrs), ">\n"});
      for (const auto& child : node.children_nodes()) {
        content += GenerateContentStructure(child, depth + 1);
      }
      content += base::StrCat({indent, "</iframe>\n"});
      return content;

    case ContentAttributeType::CONTENT_ATTRIBUTE_SVG:
      content += base::StrCat({indent, "<svg", BuildAttributes(attrs)});
      if (attrs.has_svg_data() && attrs.svg_data().has_inner_text()) {
        content += base::StrCat(
            {">", XmlEscapeAndSanitizeText(attrs.svg_data().inner_text()),
             "</svg>\n"});
      } else {
        content += " />\n";
      }
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_CANVAS:
      content +=
          base::StrCat({indent, "<canvas", BuildAttributes(attrs), " />\n"});
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_VIDEO:
      content += base::StrCat({indent, "<video"});
      if (attrs.has_video_data() && attrs.video_data().has_url()) {
        content += base::StrCat(
            {" src=\"", XmlEscapeAndSanitizeText(attrs.video_data().url()),
             "\""});
      }
      content += base::StrCat({BuildAttributes(attrs), " />\n"});
      break;

    case ContentAttributeType::CONTENT_ATTRIBUTE_UNKNOWN:
    case ContentAttributeType::
        ContentAttributeType_INT_MIN_SENTINEL_DO_NOT_USE_:
    case ContentAttributeType::
        ContentAttributeType_INT_MAX_SENTINEL_DO_NOT_USE_:
      // Skip unknown or sentinel values
      break;
  }

  // Process children for elements that don't handle them explicitly above
  for (const auto& child : node.children_nodes()) {
    content += GenerateContentStructure(child, depth + 1);
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
  std::string result = base::StrCat({kBraveUntrustedContentOpenTag, "\n"});

  result += "=== PAGE METADATA ===\n\n";

  // Add page metadata
  if (page_content.has_main_frame_data()) {
    const auto& frame_data = page_content.main_frame_data();
    if (frame_data.has_title()) {
      result += base::StrCat(
          {"PAGE TITLE: ", XmlEscapeAndSanitizeText(frame_data.title()), "\n"});
    }
    if (frame_data.has_url()) {
      result += base::StrCat(
          {"PAGE URL: ", XmlEscapeAndSanitizeText(frame_data.url()), "\n"});
    }

    if (frame_data.has_document_identifier()) {
      result += base::StrCat(
          {"PAGE ROOT DOCUMENT IDENTIFIER: ",
           XmlEscapeAndSanitizeText(
               frame_data.document_identifier().serialized_token()),
           "\n"});
    }
    result += "\n";
  }

  // Add viewport information for coordinate references
  if (page_content.has_viewport_geometry()) {
    const auto& viewport = page_content.viewport_geometry();
    result += absl::StrFormat(
        "VIEWPORT: %dx%d pixels, currently scrolled at %d,%d\n",
        viewport.width(), viewport.height(), viewport.x(), viewport.y());
  }

  std::string tree_string = GenerateContentStructure(page_content.root_node());
  if (tree_string.length() > kMaxTreeStringLength) {
    // TODO(https://github.com/brave/brave-browser/issues/49262): prioritize
    // viewport elements - the consumer can then scroll to "paginate."
    tree_string = tree_string.substr(0, kMaxTreeStringLength) + "...</root>\n";
    tree_string +=
        "PAGE STRUCTURE (XML) was too long to display. Truncated.\n\n";
  }

  result += "\n=== PAGE STRUCTURE (XML representation) ===";

  // Replace all occurances of the untrusted tag with an empty string
  base::ReplaceSubstringsAfterOffset(&tree_string, 0,
                                     kBraveUntrustedContentTagName, "");

  result += tree_string;
  result += base::StrCat({"\n", kBraveUntrustedContentCloseTag, "\n"});

  // Add usage instructions
  result += "\n=== INTERACTION INSTRUCTIONS ===\n";
  result +=
      "The page structure represents the entire page and not just the "
      "viewport. Use scroll only if neccessary or to show the user something. "
      "Use the XML attributes to guide interaction:\n";
  result +=
      "- dom_id: Use for precise element targeting but you must provide the "
      "document_identifier either from the root or from an iframe.\n";
  result +=
      "- x,y,width,height: Use the position/size only when neccessary or to "
      "infer hierarchy.\n";
  result += "- clickable: Element can be clicked\n";
  result += "- editable: Element can receive text input\n";
  // result += "- focusable=\"true\": Element can receive focus\n";
  // result +=
  //     "- role: Semantic role (header, nav, main, search, article, section, "
  //     "aside, footer, hidden, paid)\n";
  // result += "- label: Accessibility label for the element\n";

  // Convert to ContentBlocks using the existing utility
  return CreateContentBlocksForText(result);
}

}  // namespace ai_chat
