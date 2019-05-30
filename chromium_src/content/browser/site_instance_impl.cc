/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../content/browser/site_instance_impl.cc"  // NOLINT

namespace content {

// static
scoped_refptr<SiteInstanceImpl> SiteInstanceImpl::CreateForURL(
    BrowserContext* browser_context,
    const GURL& url) {
  return CreateForURL(browser_context, url, url);
}

void SiteInstanceImpl::SetSite(const GURL& url) {
  SetSite(url, url);
}

const GURL& SiteInstanceImpl::GetFirstPartyURL() const {
  return first_party_;
}

}  // namespace content
