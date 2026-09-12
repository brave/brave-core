// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js';

export class SettingsSearchPageElement extends CrLitElement {
  static get is() {
    return 'settings-search-page';
  }

  protected accessor showDialog_: boolean = false;
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-search-page': SettingsSearchPageElement;
  }
}

