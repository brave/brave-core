// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ColorChangeUpdater, COLORS_CSS_SELECTOR } from './colors_css_updater-chromium.js';

const DEFAULT_COLORS_HREF = '//theme/colors.css?sets=ui';

const oldStart = ColorChangeUpdater.prototype.start;

ColorChangeUpdater.prototype.start = function() {
  const root = (this as unknown as { root_: ShadowRoot | Document }).root_
  // Ensure that we load colors.css or the ColorChangeUpdater won't update them
  // when the theme changes.
  // We need this because we want all WebUI pages to use the dynamic colors,
  // whereas upstream only opts in specific pages (where they explicitly include
  // the CSS).
  const colorCssNode = root.querySelector(COLORS_CSS_SELECTOR);
  if (!colorCssNode) {
    const colorsCssLink = document.createElement('link');
    colorsCssLink.rel = 'stylesheet';
    colorsCssLink.type = 'text/css';
    colorsCssLink.setAttribute('href', DEFAULT_COLORS_HREF);
    if (root === document) {
      document.getElementsByTagName('body')[0]!.appendChild(colorsCssLink);
    } else {
      root.appendChild(colorsCssLink);
    }
  }

  oldStart.call(this);
};

export * from './colors_css_updater-chromium.js';
