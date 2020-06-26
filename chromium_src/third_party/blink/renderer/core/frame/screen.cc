/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/screen.h"

#define BRAVE_REGISTER_PAGEGRAPH_WEB_API(api_string) \
  { \
    brave_page_graph::PageGraph* page_graph = \
      DomWindow()->GetFrame()->GetDocument()->GetPageGraph(); \
    if (page_graph != nullptr) { \
      page_graph->RegisterWebAPICall(api_string, \
        std::vector<const String>()); \
      const String page_graph_result(std::to_string(result).c_str()); \
      page_graph->RegisterWebAPIResult(api_string, page_graph_result); \
    } \
  }

#define BRAVE_PAGEGRAPH_SCREEN_HEIGHT_WRAPPER \
    const int result = Screen::heightInternal(); \
    BRAVE_REGISTER_PAGEGRAPH_WEB_API("Screen.height") \
    return result; \
    } \
    int Screen::heightInternal() const {

#define BRAVE_PAGEGRAPH_SCREEN_WIDTH_WRAPPER \
    const int result = Screen::widthInternal(); \
    BRAVE_REGISTER_PAGEGRAPH_WEB_API("Screen.width") \
    return result; \
    } \
    int Screen::widthInternal() const {

#define BRAVE_PAGEGRAPH_SCREEN_COLORDEPTH_WRAPPER \
    const unsigned result = Screen::colorDepthInternal(); \
    BRAVE_REGISTER_PAGEGRAPH_WEB_API("Screen.colorDepth") \
    return result; \
    } \
    unsigned Screen::colorDepthInternal() const {

#define BRAVE_PAGEGRAPH_SCREEN_AVAILLEFT_WRAPPER \
    const int result = Screen::availLeftInternal(); \
    BRAVE_REGISTER_PAGEGRAPH_WEB_API("Screen.availLeft") \
    return result; \
    } \
    int Screen::availLeftInternal() const {

#define BRAVE_PAGEGRAPH_SCREEN_AVAILTOP_WRAPPER \
    const int result = Screen::availTopInternal(); \
    BRAVE_REGISTER_PAGEGRAPH_WEB_API("Screen.availTop") \
    return result; \
    } \
    int Screen::availTopInternal() const {

#define BRAVE_PAGEGRAPH_SCREEN_AVAILHEIGHT_WRAPPER \
    const int result = Screen::availHeightInternal(); \
    BRAVE_REGISTER_PAGEGRAPH_WEB_API("Screen.availHeight") \
    return result; \
    } \
    int Screen::availHeightInternal() const {

#define BRAVE_PAGEGRAPH_SCREEN_AVAILWIDTH_WRAPPER \
    const int result = Screen::availWidthInternal(); \
    BRAVE_REGISTER_PAGEGRAPH_WEB_API("Screen.availWidth") \
    return result; \
    } \
    int Screen::availWidthInternal() const {

#include "../../../../../../../third_party/blink/renderer/core/frame/screen.cc"
