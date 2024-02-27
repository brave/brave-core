/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { visualDomDiff } from 'visual-dom-diff'
import * as data from '../report/reports.json'

async function loadDocument(dir, doc, type) {
   let response = await new Promise(resolve => {
      var xhr = new XMLHttpRequest()
      xhr.open("GET", '/report/' + dir +"/" + doc, true)
      xhr.responseType = type
      xhr.onload = function(e) {
        resolve(xhr.response)
      };
      xhr.onerror = function () {
        resolve(undefined);
      };
      xhr.send()
   })
   return response
}

function getShadowRoot(e) {
  if (e.shadowRoot) {
    return e.shadowRoot
  }
  return e.attachShadow({mode: 'open'})
}

async function showReport(val) {
  const original = await loadDocument(val, 'original.html', 'document')
  original.body.hidden = false
  const originalNode = document.getElementById('original')
  getShadowRoot(originalNode).replaceChildren(original.firstChild)

  const changed = await loadDocument(val, 'changed.html', 'document')
  changed.body.hidden = false
  const changedNode = document.getElementById('changed')
  getShadowRoot(changedNode).replaceChildren(changed.firstChild)

  const pageUrl = await loadDocument(val, 'page.url', 'text')
  const pageUrlNode = document.getElementById('page_url')
  pageUrlNode.href = pageUrl
  pageUrlNode.text = pageUrl

  const diffNode = document.getElementById('diff')
  getShadowRoot(diffNode).replaceChildren(
    visualDomDiff(getShadowRoot(originalNode).firstChild,
                  getShadowRoot(changedNode).firstChild))
}

window.addEventListener('load', function () {
    const reportsSelect = document.getElementById('reports')
    for (const r of data.default.reports) {
        const opt = document.createElement('option')
        opt.value = r
        opt.text = r
        reportsSelect.appendChild(opt)
    }
    reportsSelect.onchange = (e) => showReport(e.target.value)

    showReport(reportsSelect.value)
})
