// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

(() => {
  "use strict"

  /**
   * Send the results of farbled APIs to iOS so it can see if its properly farbled
   * @param {Array} hiddenIds All the ids that are hidden
   * @param {Array} unhiddenIds All the ids that are not hidden
   * @returns A Promise that resolves new hide selectors
   */
  const sendTestResults = (hiddenIds, unhiddenIds) => {
    return webkit.messageHandlers['SendCosmeticFiltersResult'].postMessage({
      'hiddenIds': hiddenIds,
      'unhiddenIds': unhiddenIds
    })
  }

  const getHideResults = () => {
    const elements = document.querySelectorAll('[id]')
    const results = {
      hiddenIds: [],
      unhiddenIds: []
    }

    elements.forEach((node) => {
      if (!node.hasAttribute('id')) {
        return
      }

      if (window.getComputedStyle(node).display === 'none') {
        results.hiddenIds.push(node.id)
      } else {
        results.unhiddenIds.push(node.id)
      }
    })

    return results
  }

  let results = getHideResults()
  sendTestResults(results.hiddenIds, results.unhiddenIds)
})()
