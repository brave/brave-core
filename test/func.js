/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import chromedriver from 'chromedriver'
import webdriver from 'selenium-webdriver'

export function delay (time) {
  return new Promise(resolve => setTimeout(resolve, time))
}

let chromedriverIsStarted = false
export function startChromeDriver () {
  if (chromedriverIsStarted) return Promise.resolve()
  chromedriver.start()
  process.on('exit', () => {
    chromedriver.stop()
  })
  chromedriverIsStarted = true
  return delay(1000)
}

export function buildWebDriver (extPath) {
  return new webdriver.Builder()
    .usingServer('http://localhost:9515')
    .withCapabilities({
      chromeOptions: {
        args: [`load-extension=${extPath}`],
        // TODO: Set this properly for platforms other than macOS
        binary: '../../../out/Release/Brave.app/Contents/MacOS/Brave'
      }
    })
    .forBrowser('chrome')
    .build()
}
