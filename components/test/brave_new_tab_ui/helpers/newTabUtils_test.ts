/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { isHttpOrHttps, getCharForSite } from '../../../brave_new_tab_ui/helpers/newTabUtils'

describe('new tab util files tests', () => {
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

  describe('getCharForSite', () => {
    it('returns the first letter of a given URL without subdomains', () => {
      const url = { url: 'https://brave.com' }
      expect(getCharForSite(url)).toBe('B')
    })
    it('returns the first letter of a given URL with subdomains', () => {
      const url = { url: 'https://awesome-sub-domain.brave.com' }
      expect(getCharForSite(url)).toBe('A')
    })
    it('returns the first letter of a given URL with ports', () => {
      const url = { url: 'https://brave.com:9999' }
      expect(getCharForSite(url)).toBe('B')
    })
    it('returns the first letter of a given URL with paths', () => {
      const url = { url: 'https://brave.com/hello-test/' }
      expect(getCharForSite(url)).toBe('B')
    })
    it('returns the first letter of a given URL with queries', () => {
      const url = { url: 'https://brave.com/?randomId' }
      expect(getCharForSite(url)).toBe('B')
    })
    it('returns the first letter of a given URL with parameters', () => {
      const url = { url: 'https://brave.com/?randomId=123123123' }
      expect(getCharForSite(url)).toBe('B')
    })
    it('returns the first letter of a given URL with fragments', () => {
      const url = { url: 'https://brave.com/?randomId=123123123&hl=en#00h00m10s' }
      expect(getCharForSite(url)).toBe('B')
    })
  })
})
