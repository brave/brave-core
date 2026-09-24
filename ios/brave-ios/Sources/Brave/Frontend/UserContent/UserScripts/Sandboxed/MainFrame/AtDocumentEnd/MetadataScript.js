// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

"use strict";

function MetadataWrapper() {
  this.getMetadata = function() {
    const searchEl = document.querySelector(
      'link[type="application/opensearchdescription+xml"]'
    );
    const search = searchEl
      ? { title: searchEl.title, href: searchEl.href }
      : null;

    const rules = [
      'link[type="application/rss+xml"]',
      'link[type="application/atom+xml"]',
      'link[rel="alternate"][type="application/json"]',
    ].join(', ');
    const feeds = Array.from(document.querySelectorAll(rules))
      .map(link => ({ href: link.href, title: link.title }));

    return { search, feeds };
  };
}

Object.defineProperty(window.__firefox__, 'metadata', {
  enumerable: false,
  configurable: false,
  writable: false,
  value: Object.freeze(new MetadataWrapper())
});
