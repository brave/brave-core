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

const findShieldsPanel = async driver => {
  await delay(1000)
  return (await driver.findElements(webdriver.By.css(`[data-test-id="brave-shields-panel"]`)))[0]
}

describe('browser action page', function test () {
  let driver
  this.timeout(15000)

  before(async () => {
    await startChromeDriver()
    const extPath = path.resolve('build')
    driver = buildWebDriver(extPath)
    await driver.get('chrome://extensions-frame')

    await delay(1000)
    const handle = await driver.getWindowHandle()
    await driver.switchTo().window(handle)

    const elems = await driver.findElements(webdriver.By.xpath(
      '//div[contains(@class, "extension-list-item-wrapper") and ' +
      `.//h2[contains(text(), "${extensionName}")]]`
    ))
    const extensionId = await elems[0].getAttribute('id')
    await driver.get(`chrome-extension://${extensionId}/braveShieldsPanel.html`)
  })

  after(async () => {
    await driver.quit()
    process.exit()
  })

  it('opens Brave Shields Panel', async () => {
    const title = await driver.getTitle()
    assert.equal(title, 'Brave Shields Panel')
  })

  it('contains the shields header', async () => {
    const shieldsPanel = await findShieldsPanel(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('Shields'))
  })

  it('contains Ads and Trackers Blocked stats', async () => {
    const shieldsPanel = await findShieldsPanel(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('Ads and Trackers Blocked'))
  })

  it('contains HTTPS Upgrade stats', async () => {
    const shieldsPanel = await findShieldsPanel(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('HTTPS Upgrades'))
  })

  it('contains Scripts Blocked stats', async () => {
    const shieldsPanel = await findShieldsPanel(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('Scripts Blocked'))
  })

  it('contains the Advanced Controls toggle', async () => {
    const shieldsPanel = await findShieldsPanel(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('Advanced Controls'))
  })

  it('contains Ad Control', async () => {
    const shieldsPanel = await findShieldsPanel(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('Ad Control'))
  })

  it('contains Cookie Control', async () => {
    const shieldsPanel = await findShieldsPanel(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('Cookie Control'))
  })

  it('contains HTTPS Everywhere', async () => {
    const shieldsPanel = await findShieldsPanel(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('HTTPS Everywhere'))
  })

  it('contains Block Scripts', async () => {
    const shieldsPanel = await findShieldsPanel(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('Block Scripts'))
  })

  it('contains Fingerprinting Protection', async () => {
    const shieldsPanel = await findShieldsPanel(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('Fingerprinting Protection'))
  })

  it('contains Block Phishing/Malware', async () => {
    const shieldsPanel = await findShieldsPanel(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('Block Phishing/Malware'))
  })
})
