/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_LIBXML_UTILS_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_LIBXML_UTILS_H_

#include <libxml/xmlstring.h>

#include <string_view>

#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave_page_graph {

// Creates valid UTF-8 strings to use in libxml API (null-terminated and valid
// UTF-8 characters).
class XmlUtf8String {
 public:
  explicit XmlUtf8String(std::string_view str);
  explicit XmlUtf8String(const String& str);
  explicit XmlUtf8String(int value);
  ~XmlUtf8String();

  XmlUtf8String(const XmlUtf8String&) = delete;
  XmlUtf8String& operator=(const XmlUtf8String&) = delete;

  xmlChar* get() const { return xml_string_; }

 private:
  xmlChar* xml_string_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_LIBXML_UTILS_H_
