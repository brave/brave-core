// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Convert to Polymer class and remove ts-nocheck

import {Polymer} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {getTemplate} from './brave_history_item.html.js'

Polymer({
  is: 'brave-history-item',
  _template: getTemplate()
})
