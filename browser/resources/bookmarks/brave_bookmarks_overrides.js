// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
const template =  html`
<dom-module id="brave-bookmarks-shared-style">{__html_template__}</dom-module>
`;
document.body.appendChild(template.content.cloneNode(true));
