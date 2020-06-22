/* vim: set ts=2 sts=2 sw=2 et tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const {getMetadata:metadataparser, metadataRuleSets} = require("page-metadata-parser");

function MetadataWrapper() {
  this.getMetadata = function() {
    const customRuleSets = metadataRuleSets;
    customRuleSets.icon.rules = [
       ['link[rel="icon" i]', element => element.getAttribute('href')],
       ['link[rel="fluid-icon"]', element => element.getAttribute('href')],
       ['link[rel="shortcut icon"]', element => element.getAttribute('href')],
       ['link[rel="Shortcut Icon"]', element => element.getAttribute('href')],
    ];
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
    return metadataparser(window.document, document.URL, customRuleSets);
  };
}

Object.defineProperty(window.__firefox__, 'metadata', {
  enumerable: false,
  configurable: false,
  writable: false,
  value: Object.freeze(new MetadataWrapper(metadataparser))
});
