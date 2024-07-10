/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

self.testing = (() => {

  function waitFor(name, fn) {
    let warnAfter = 2000

    return new Promise((resolve) => {
      let start = Date.now()
      let interval = 30

      function check() {
        const result = fn()
        if (result != null) {
          resolve(result)
          return
        }
        setTimeout(check, interval)
        interval *= 2

        if (Date.now() - start > warnAfter) {
          console.warn(`Still waiting for ${name}`)
        }
      }

      check()
    })
  }

  function waitForElement(selector, root = document) {
    return waitFor(`element (${selector})`, () => root.querySelector(selector))
  }

  async function waitForShadowElement(selectors) {
    let elem = undefined
    for (let i = 0; i < selectors.length; ++i) {
      if (i > 0) {
        elem = elem.shadowRoot
        if (!elem) {
          let path = selectors.slice(0, i).join(" >>> ")
          throw new Error(`Element (${path}) does not have a shadow root`)
        }
      }
      elem = await waitForElement(selectors[i], elem)
    }
    return elem
  }

  let requestHandlers = []

  function handleRequest(url, method) {
    let urlObject = new URL(url)
    for (let handler of requestHandlers) {
      let result = handler(urlObject, method)
      if (result) {
        if (typeof result[1] === 'object') {
          result[1] = JSON.stringify(result[1])
        }
        return result
      }
    }
    console.warn(`Request <${method} ${url}> not handled`)
    return [404, ""]
  }

  function addRequestHandler(handler) {
    requestHandlers.push(handler)
  }

  let tests = []

  async function runTests() {
    let currentTests = [...tests]
    tests = []
    for (let test of currentTests) {
      await test()
    }
  }

  function addTest(fn) {
    tests.push(fn)
  }

  return {
    // Test Runner API
    handleRequest,
    runTests,

    // Test API
    addRequestHandler,
    addTest,
    waitFor,
    waitForElement,
    waitForShadowElement
  }

})()
