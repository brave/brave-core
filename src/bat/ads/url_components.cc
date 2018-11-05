/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/url_components.h"

namespace ads {

UrlComponents::UrlComponents() :
    url(""),
    scheme(""),
    user(""),
    hostname(""),
    port(""),
    query(""),
    fragment(""),
    absolute_path(false) {}

UrlComponents::UrlComponents(const UrlComponents& components) :
    url(components.url),
    scheme(components.scheme),
    user(components.user),
    hostname(components.hostname),
    port(components.port),
    query(components.query),
    fragment(components.fragment),
    absolute_path(components.absolute_path) {}

UrlComponents::~UrlComponents() {}

}  // namespace ads
