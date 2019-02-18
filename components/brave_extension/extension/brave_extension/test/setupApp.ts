/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as jsdom from 'jsdom'
import { getMockChrome } from './testData'

const { JSDOM } = jsdom
const { document } = (new JSDOM('<!doctype html><html><body></body></html>')).window

import { configure } from 'enzyme'
import * as Adapter from 'enzyme-adapter-react-16'

global.document = document
global.window = document.defaultView
global.navigator = global.window.navigator
global.HTMLElement = global.window.HTMLElement
if (global.chrome === undefined) {
  global.chrome = getMockChrome()
}
// mocks rAF to suppress React warning while testing
global.requestAnimationFrame = function (cb: () => void) {
  return setTimeout(cb, 0)
}

configure({ adapter: new Adapter() })