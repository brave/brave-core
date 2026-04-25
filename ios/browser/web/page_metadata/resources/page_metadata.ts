// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {CrWebApi, gCrWeb} from '//ios/web/public/js_messaging/resources/gcrweb.js';

function getMetadata(): string {
  const searchEl = document.querySelector(
      'link[type="application/opensearchdescription+xml"]');
  const search = searchEl ? {
    title: (searchEl as HTMLLinkElement).title,
    href: (searchEl as HTMLLinkElement).href,
  } : null;

  const feedNodes = document.querySelectorAll(
      'link[type="application/rss+xml"], ' +
      'link[type="application/atom+xml"], ' +
      'link[rel="alternate"][type="application/json"]');
  const feeds = Array.from(feedNodes).map(link => ({
    href: (link as HTMLLinkElement).href,
    title: (link as HTMLLinkElement).title,
  }));

  return JSON.stringify({search, feeds});
}

const pageMetadataApi = new CrWebApi('pageMetadata');
pageMetadataApi.addFunction('getMetadata', getMetadata);
gCrWeb.registerApi(pageMetadataApi);
