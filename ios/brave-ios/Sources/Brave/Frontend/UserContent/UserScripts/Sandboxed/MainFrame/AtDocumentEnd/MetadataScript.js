// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

"use strict";

const {getMetadata:metadataparser, metadataRuleSets} = require("page-metadata-parser");

function MetadataWrapper() {
  this.getMetadata = function() {
    const customRuleSets = metadataRuleSets;
    customRuleSets.icon.defaultValue = () => "";
    customRuleSets.icon.rules = [
       ['link[rel="icon" i]', element => element.getAttribute('href')],
       ['link[rel="fluid-icon"]', element => element.getAttribute('href')],
       ['link[rel="shortcut icon"]', element => element.getAttribute('href')],
       ['link[rel="Shortcut Icon"]', element => element.getAttribute('href')],
    ];
    customRuleSets.search = {
      rules: [
        ['link[type="application/opensearchdescription+xml"]', element => { return { title: element.title, href: element.href } }]
      ]
    };
    customRuleSets.largeIcon = {
      rules: [
        ['link[rel="apple-touch-icon"]', element => element.getAttribute('href')],
        ['link[rel="apple-touch-icon-precomposed"]', element => element.getAttribute('href')]
      ],
      defaultValue: null,
      scorers: [
        // Handles the case where multiple icons are listed with specific sizes ie
        // <link rel="icon" href="small.png" sizes="16x16">
        // <link rel="icon" href="large.png" sizes="32x32">
        (element, score) => {
          // We want to get as close to 180x180
          const goal = 180 * 180;
          const sizes = element.getAttribute('sizes');

          if (sizes) {
            const sizeMatches = sizes.match(/\d+/g);

            if (sizeMatches) {
              const sizeMulti = sizeMatches.reduce((a, b) => a * b);
              return 1.0 - (Math.abs(sizeMulti - goal)) / goal;
            }
          }
          return 0.01
        }
      ],
      processors: customRuleSets.icon.processors
    };
    const data = metadataparser(window.document, document.URL, customRuleSets);
    // Since we want to obtain multiple feeds, we are doing a separate query.
    // `page-metadata-parser` only allows a single result from rules passed into `getMetadata`
    data.feeds = function() {
      const rules = 'link[type="application/rss+xml"], link[type="application/atom+xml"], link[rel="alternate"][type="application/json"]';
      const nodes = window.document.querySelectorAll(rules);
      return Array.from(nodes).map(link => {return {href: link.href, title: link.title};});
    }();
    return data
  };
}

Object.defineProperty(window.__firefox__, 'metadata', {
  enumerable: false,
  configurable: false,
  writable: false,
  value: Object.freeze(new MetadataWrapper(metadataparser))
});
