/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {html} from '//resources/lit/v3_0/lit.rollup.js';

import type {BraveEducationAppElement} from './brave_education_app.js';

export function getHtml(this: BraveEducationAppElement) {
  return this.url_ ? html`<iframe id="content" src="${this.url_}"></iframe>` :
                     '';
}
