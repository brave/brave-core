// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'

function queryInAllTemplates(
  fragment: DocumentFragment,
  selector: string,
  onFound: (element: Element) => void) {
  if (!fragment) {
    return
  }
  for (const foundElement of fragment.querySelectorAll(selector)) {
    onFound(foundElement)
  }
  for (const foundTemplate of fragment.querySelectorAll('template')) {
    if (foundTemplate.content) {
      queryInAllTemplates(foundTemplate.content, selector, onFound)
    }
  }
}

RegisterPolymerTemplateModifications({
  'settings-default-browser-page': (templateContent) => {
    // Stop both versions thinking they are the first, since this item is added
    // to Brave's "Get Started" section.
    queryInAllTemplates(templateContent, '.cr-row.first',
      element => {
        element.classList.remove('first')
    })
  }
})
