/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as assert from 'assert'
import { isHttpOrHttps } from '../../../app/helpers/urlUtils'

describe('urlUtils test', function () {
  describe('isHttpOrHttps', function () {
    it('matches http when defined as a protocol type', function () {
      const url = 'http://some-boring-unsafe-website.com'
      assert.equal(isHttpOrHttps(url), true)
    })
    it('matches https when defined as a protocol type', function () {
      const url = 'https://some-nice-safe-website.com'
      assert.equal(isHttpOrHttps(url), true)
    })
    it('does not match http when defined as an origin', function () {
      const url = 'file://http.some-website-tricking-you.com'
      assert.equal(isHttpOrHttps(url), false)
    })
    it('does not match https when defined as an origin', function () {
      const url = 'file://https.some-website-tricking-you.com'
      assert.equal(isHttpOrHttps(url), false)
    })
    it('does not match other protocol', function () {
      const url = 'ftp://some-old-website.com'
      assert.equal(isHttpOrHttps(url), false)
    })
    it('does not match when url is not defined', function () {
      const url = undefined
      assert.equal(isHttpOrHttps(url), false)
    })
    it('matches uppercase http', function () {
      const url = 'HTTP://SCREAMING-SAFE-WEBSITE.COM'
      assert.equal(isHttpOrHttps(url), true)
    })
    it('matches uppercase https', function () {
      const url = 'HTTP://SCREAMING-UNSAFE-WEBSITE.COM'
      assert.equal(isHttpOrHttps(url), true)
    })
  })
})