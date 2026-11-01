// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle, mangleAll } from 'lit_mangler'

const hide = (root: DocumentFragment, id: string) => {
  const element = root.querySelector(`#${id}`)
  if (!element) {
    throw new Error(`[Settings] Security page: couldn't find #${id}`)
  }
  element.setAttribute('hidden', '')
}

mangle((root) => {
  hide(root, 'safeBrowsingEnhanced')
  hide(root, 'passwordsLeakToggle')
  hide(root, 'advancedProtectionProgramLink')

  // The standard protection options are hidden, so drop the collapse arrow
  // and separator.
  const standard = root.querySelector('#safeBrowsingStandard')
  if (!standard) {
    throw new Error(
      `[Settings] Security page: couldn't find #safeBrowsingStandard`)
  }
  standard.removeAttribute('?no-collapse')
  standard.setAttribute('no-collapse', '')

  // Make loadTimeData available to the template, via the imports
  // html_to_wrapper carries over to the generated security_page.html.ts.
  const imports = Array.from(root.childNodes).find(
    (node): node is Comment =>
      node.nodeType === node.COMMENT_NODE
      && (node as Comment).data.startsWith(' #html_wrapper_imports_start'))
  if (!imports) {
    throw new Error(
      `[Settings] Security page: couldn't find #html_wrapper_imports_start`)
  }
  imports.data = imports.data.replace(
    '\n#html_wrapper_imports_end',
    `\nimport {loadTimeData} from '../../i18n_setup.js';`
      + '\n#html_wrapper_imports_end')
})

mangleAll(
  (root) => hide(root, 'safeBrowsingReportingToggle'),
  (t) => t.text.includes('id="safeBrowsingReportingToggle"'),
)

// Brave's HTTPS upgrade setting supersedes the HTTPS-only mode toggle.
mangleAll(
  (root) => {
    const toggle = root.querySelector('#httpsOnlyModeToggle')
    if (!toggle) {
      throw new Error(
        `[Settings] Security page: couldn't find #httpsOnlyModeToggle`)
    }
    // '?' isn't valid in an attribute name for setAttribute(), so the Lit
    // binding is added by re-parsing the element through outerHTML.
    const tagNameEnd = toggle.tagName.length + 1
    const outerHtml = toggle.outerHTML
    // Mangling happens at build time, over the template shipped with the
    // browser, so there is no untrusted input to sanitize here.
    // eslint-disable-next-line no-unsanitized/property
    toggle.outerHTML = outerHtml.slice(0, tagNameEnd)
      + ` ?hidden="\${loadTimeData.getBoolean('isHttpsByDefaultEnabled')}"`
      + outerHtml.slice(tagNameEnd)
  },
  (t) => t.text.includes('id="httpsOnlyModeToggle"'),
)
