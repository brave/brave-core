// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'

function queryInAllTemplates(element, selector, onFound) {
  if (!element) {
    return
  }
  for (const foundElement of element.querySelectorAll(selector)) {
    onFound(foundElement)
  }
  for (const foundTemplate of element.querySelectorAll('template')) {
    if (foundTemplate.content) {
      queryInAllTemplates(foundTemplate.content, selector, onFound)
    }
  }
}

RegisterPolymerTemplateModifications({
  'settings-default-browser-page': (templateContent) => {
    // Stop both versions thinking they are the first, since this item is added
    // to Brave's "Get Started" section.
    queryInAllTemplates(templateContent, '.cr-row.first', function (element) {
      element.classList.remove('first')
    })
  }
})
