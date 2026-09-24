// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ThemeHueSliderDialogElement } from './theme_hue_slider_dialog-chromium.js';

const showAtChromium = ThemeHueSliderDialogElement.prototype.showAt;

// Upstream showAt() only clamps the popup vertically within the window. On
// narrow WebUI surfaces (e.g. the 512px wide profile customization modal) the
// right-aligned popup can overflow horizontally and get clipped, so re-clamp
// its left position within the viewport after upstream positioning runs.
ThemeHueSliderDialogElement.prototype.showAt = function(
    this: ThemeHueSliderDialogElement, anchor: HTMLElement) {
  showAtChromium.call(this, anchor);

  const padding = 8;
  const dialog = this.$.dialog;
  const left = parseFloat(dialog.style.left);
  dialog.style.left = `${
      Math.max(
          padding,
          Math.min(left, window.innerWidth - dialog.offsetWidth - padding))}px`;
};

export * from './theme_hue_slider_dialog-chromium.js';
