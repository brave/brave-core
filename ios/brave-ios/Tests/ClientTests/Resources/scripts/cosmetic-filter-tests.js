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
  const sendTestResults = (results) => {
    return webkit.messageHandlers['SendCosmeticFiltersResult'].postMessage({
      'hiddenIds': results.hiddenIds,
      'unhiddenIds': results.unhiddenIds,
      'removedClass': results.removedClass,
      'removedAttribute': results.removedAttribute,
      'removedElement': results.removedElement,
      'styledElement': results.styledElement
    })
  }

  const sendDebugMessage = (message) => {
    return webkit.messageHandlers['LoggingHandler'].postMessage(message)
  }

  const getHideResults = () => {
    const elements = document.querySelectorAll('[id]')
    const results = {
      hiddenIds: [],
      unhiddenIds: [],
      removedElement: true,
      removedClass: false,
      removedAttribute: false,
      styledElement: false
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

      if (node.id === 'test-remove-element') {
        results.removedElement = false
      }
      
      if (node.id === 'test-remove-class') {
        results.removedClass = !node.classList.contains('test')
      }

      if (node.id === 'test-remove-attribute') {
        results.removedAttribute = !node.hasAttribute('test')
      }

      if (node.id === 'test-style-element') {
        results.styledElement = window.getComputedStyle(node).backgroundColor === "rgb(255, 0, 0)"
      }
    })

    return results
  }

  let results = getHideResults()
  sendTestResults(results)
})()
