// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_HTML_PARSING_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_HTML_PARSING_H_

#include <string>
#include <vector>

class GURL;

namespace brave_news {

std::vector<GURL> GetFeedURLsFromHTMLDocument(const std::string& charset,
                                              const std::string& html_body,
                                              const GURL& html_url);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_HTML_PARSING_H_
