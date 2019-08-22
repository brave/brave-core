/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_UTILITIES_DIGEST_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_UTILITIES_DIGEST_H_

#include <string>

namespace blink {

class Image;
class CSSStyleSheetResource;
class ScriptResource;
class Resource;

}

namespace brave_page_graph {

std::string ImageDigest(blink::Image* image);
std::string ScriptDigest(blink::ScriptResource* resource);
std::string StyleSheetDigest(blink::CSSStyleSheetResource* resource);
std::string ResourceDigest(blink::Resource* resource);

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_UTILITIES_DIGEST_H_
