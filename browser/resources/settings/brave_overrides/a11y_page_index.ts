// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// <if expr="is_linux">
import { RegisterPolymerTemplateModifications } from 'chrome://resources/brave/polymer_overriding.js';

RegisterPolymerTemplateModifications({
  'settings-a11y-page-index': (templateContent) => {
    const captionsRoute = templateContent.
      querySelector('#captions')
    if (!captionsRoute) {
      throw new Error('Could not find #captions')
    }
    captionsRoute.remove()
  },
});
// </if>
