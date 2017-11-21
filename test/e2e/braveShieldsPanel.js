/* global describe, it, before, after */

import path from 'path'
import webdriver from 'selenium-webdriver'
import { delay, startChromeDriver, buildWebDriver } from '../func'
import manifest from '../../app/manifest.prod.json'
import assert from 'assert'

const extensionName = manifest.name

const findShieldsPanel = driver =>
  driver.findElements(webdriver.By.css(`[data-test-id="brave-shields-panel"]`))

const toggleShields = async (driver) => {
  await delay(1000)
  return (await findShieldsPanel(driver))[0]
}

describe('window (popup) page', function test () {
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
    await driver.get(`chrome-extension://${extensionId}/window.html`)
  })

  after(async () => {
    await driver.quit()
    process.exit()
  })

  it('opens Brave Shields Panel', async () => {
    const title = await driver.getTitle()
    assert.equal(title, 'Brave Shields Panel')
  })

  it('contains adBlock:allow', async () => {
    const shieldsPanel = await toggleShields(driver)
    const text = await shieldsPanel.getText()
    assert(text.includes('adBlock: allow'))
  })
})
