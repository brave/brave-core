/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { isHttpOrHttps, hasPortNumber } from '../../../app/helpers/urlUtils'

describe('urlUtils test', () => {
  describe('isHttpOrHttps', () => {
    it('matches http when defined as a protocol type', () => {
      const url = 'http://some-boring-unsafe-website.com'
      expect(isHttpOrHttps(url)).toBe(true)
    })
    it('matches https when defined as a protocol type', () => {
      const url = 'https://some-nice-safe-website.com'
      expect(isHttpOrHttps(url)).toBe(true)
    })
    it('does not match http when defined as an origin', () => {
      const url = 'file://http.some-website-tricking-you.com'
      expect(isHttpOrHttps(url)).toBe(false)
    })
    it('does not match https when defined as an origin', () => {
      const url = 'file://https.some-website-tricking-you.com'
      expect(isHttpOrHttps(url)).toBe(false)
    })
    it('does not match other protocol', () => {
      const url = 'ftp://some-old-website.com'
      expect(isHttpOrHttps(url)).toBe(false)
    })
    it('does not match when url is not defined', () => {
      const url = undefined
      expect(isHttpOrHttps(url)).toBe(false)
    })
    it('matches uppercase http', () => {
      const url = 'HTTP://SCREAMING-SAFE-WEBSITE.COM'
      expect(isHttpOrHttps(url)).toBe(true)
    })
    it('matches uppercase https', () => {
      const url = 'HTTP://SCREAMING-UNSAFE-WEBSITE.COM'
      expect(isHttpOrHttps(url)).toBe(true)
    })
  })
  describe('hasPortNumber', () => {
    it('not a port number if # is located in front of :XXXX', () => {
      const url = 'http://brianbondy.com#:8080'
      expect(hasPortNumber(url)).toBe(false)
    })
    it('port number if # is not existed in front of :XXXX', () => {
      const url = 'http://brianbondy.com:8080'
      expect(hasPortNumber(url)).toBe(true)
      expect(isHttpOrHttps(url)).toBe(true)
    })
  })
})
