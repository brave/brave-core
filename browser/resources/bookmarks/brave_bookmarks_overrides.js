// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

const template = document.createElement('template');
template.innerHTML = `
<dom-module id="brave-bookmarks-shared-style">{__html_template__}</dom-module>
`;
document.body.appendChild(template.content.cloneNode(true));
