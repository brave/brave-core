/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import path from 'path'
import webdriver from 'selenium-webdriver'
import { delay, startChromeDriver, buildWebDriver } from '../func'
import manifest from '../../app/manifest.prod.json'
import assert from 'assert'

const extensionName = manifest.name

const findNewTabPageContainer = async driver => {
  await delay(1000)
  return (await driver.findElements(webdriver.By.css(`[data-test-id="new-tab-page"]`)))[0]
}

describe('new tab page', function test () {
  let driver
  this.timeout(15000)

  before(async () => {
    await startChromeDriver()
    const extPath = path.resolve('build')
    driver = buildWebDriver(extPath)
    await driver.get('chrome://extensions-frame')
    const elems = await driver.findElements(webdriver.By.xpath(
      '//div[contains(@class, "extension-list-item-wrapper") and ' +
      `.//h2[contains(text(), "${extensionName}")]]`
    ))
    const extensionId = await elems[0].getAttribute('id')
    await driver.get(`chrome-extension://${extensionId}/newTabPage.html`)
  })

  after(async () => {
    await driver.quit()
    process.exit()
  })

  it('opens new tab page', async () => {
    const title = await driver.getTitle()
    assert.equal(title, 'Brave New Tab Page')
  })

  it('contains \'Settings\'', async () => {
    const newTabPageContainer = await findNewTabPageContainer(driver)
    const text = await newTabPageContainer.getText()
    assert(text.includes('Settings'))
  })
})
