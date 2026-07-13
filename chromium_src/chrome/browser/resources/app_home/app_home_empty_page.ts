// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {AppHomeEmptyPageElement} from './app_home_empty_page-chromium.js';

const originalConnectedCallback =
  AppHomeEmptyPageElement.prototype.connectedCallback;

AppHomeEmptyPageElement.prototype.connectedCallback = function (
  this: AppHomeEmptyPageElement,
) {
  originalConnectedCallback?.call(this);

  // Let the page background follow the active browser theme instead of
  // forcing the dark-only app home appearance.
  document.documentElement.style.colorScheme = 'light dark';
  document.documentElement.style.backgroundColor = 'Canvas';
  document.documentElement.style.color = 'CanvasText';
  document.body.style.backgroundColor = 'Canvas';
  document.body.style.color = 'CanvasText';
};

export * from './app_home_empty_page-chromium.js';
