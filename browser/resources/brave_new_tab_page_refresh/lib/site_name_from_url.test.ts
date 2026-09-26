/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { generateSiteNameFromUrl } from './site_name_from_url'

describe('generateSiteNameFromUrl', () => {
  describe('empty and invalid input', () => {
    test('returns empty string for empty input', () => {
      expect(generateSiteNameFromUrl('')).toBe('')
    })

    test('returns empty string for whitespace-only input', () => {
      expect(generateSiteNameFromUrl('   ')).toBe('')
    })

    test('returns empty string for text with no recognizable host', () => {
      expect(generateSiteNameFromUrl('not a url!!!')).toBe('')
    })
  })

  describe('special hosts', () => {
    test('handles bare localhost', () => {
      expect(generateSiteNameFromUrl('localhost')).toBe('Localhost')
    })

    test('handles localhost with port', () => {
      expect(generateSiteNameFromUrl('localhost:3000')).toBe('Localhost')
    })

    test('handles localhost with scheme', () => {
      expect(generateSiteNameFromUrl('http://localhost:8080')).toBe('Localhost')
    })

    test('returns raw host for IPv4 address', () => {
      expect(generateSiteNameFromUrl('192.168.1.1')).toBe('192.168.1.1')
    })

    test('returns raw host for IPv4 with port', () => {
      expect(generateSiteNameFromUrl('http://192.168.1.1:8080')).toBe('192.168.1.1')
    })
  })

  describe('brand capitalization', () => {
    test('returns GitHub for github.com', () => {
      expect(generateSiteNameFromUrl('github.com')).toBe('GitHub')
    })

    test('returns YouTube for youtube.com', () => {
      expect(generateSiteNameFromUrl('youtube.com')).toBe('YouTube')
    })

    test('returns LinkedIn for linkedin.com', () => {
      expect(generateSiteNameFromUrl('linkedin.com')).toBe('LinkedIn')
    })

    test('returns OpenAI for openai.com', () => {
      expect(generateSiteNameFromUrl('openai.com')).toBe('OpenAI')
    })

    test('returns LeetCode for leetcode.com', () => {
      expect(generateSiteNameFromUrl('leetcode.com')).toBe('LeetCode')
    })

    test('returns Y Combinator for news.ycombinator.com', () => {
      expect(generateSiteNameFromUrl('news.ycombinator.com')).toBe(
        'Y Combinator',
      )
    })
  })

  describe('www prefix stripping', () => {
    test('strips www. before applying brand capitalization', () => {
      expect(generateSiteNameFromUrl('www.github.com')).toBe('GitHub')
    })

    test('strips www. before title-casing', () => {
      expect(generateSiteNameFromUrl('https://www.example.com')).toBe('Example')
    })
  })

  describe('title-cased names', () => {
    test('title-cases a simple domain', () => {
      expect(generateSiteNameFromUrl('example.com')).toBe('Example')
    })

    test('converts hyphens to spaces and title-cases each word', () => {
      expect(generateSiteNameFromUrl('my-site.com')).toBe('My Site')
    })

    test('converts underscores to spaces and title-cases each word', () => {
      expect(generateSiteNameFromUrl('my_blog.io')).toBe('My Blog')
    })

    test('handles multiple consecutive hyphens', () => {
      expect(generateSiteNameFromUrl('foo--bar.dev')).toBe('Foo Bar')
    })
  })

  describe('TLD stripping', () => {
    test('strips .com', () => {
      expect(generateSiteNameFromUrl('google.com')).toBe('Google')
    })

    test('strips .org', () => {
      expect(generateSiteNameFromUrl('wikipedia.org')).toBe('Wikipedia')
    })

    test('strips .io', () => {
      expect(generateSiteNameFromUrl('linear.io')).toBe('Linear')
    })

    test('strips .dev', () => {
      expect(generateSiteNameFromUrl('web.dev')).toBe('Web')
    })

    test('strips .me', () => {
      expect(generateSiteNameFromUrl('portfolio.me')).toBe('Portfolio')
    })

    test('strips .co.uk multi-part TLD', () => {
      expect(generateSiteNameFromUrl('bbc.co.uk')).toBe('Bbc')
    })

    test('strips .com.au multi-part TLD', () => {
      expect(generateSiteNameFromUrl('news.com.au')).toBe('News')
    })

    test('strips .co.in multi-part TLD', () => {
      expect(generateSiteNameFromUrl('flipkart.co.in')).toBe('Flipkart')
    })
  })

  describe('subdomain handling', () => {
    test('ignores subdomains and uses the SLD as the base name', () => {
      expect(generateSiteNameFromUrl('mail.google.com')).toBe('Google')
    })

    test('ignores deep subdomains', () => {
      expect(generateSiteNameFromUrl('docs.support.example.com')).toBe('Example')
    })
  })

  describe('URL variants', () => {
    test('handles URL without scheme', () => {
      expect(generateSiteNameFromUrl('github.com')).toBe('GitHub')
    })

    test('handles https URL', () => {
      expect(generateSiteNameFromUrl('https://github.com')).toBe('GitHub')
    })

    test('handles http URL', () => {
      expect(generateSiteNameFromUrl('http://github.com')).toBe('GitHub')
    })

    test('ignores URL path', () => {
      expect(generateSiteNameFromUrl('https://github.com/user/repo')).toBe(
        'GitHub',
      )
    })

    test('ignores query string', () => {
      expect(generateSiteNameFromUrl('https://example.com?q=test')).toBe(
        'Example',
      )
    })

    test('trims leading/trailing whitespace from input', () => {
      expect(generateSiteNameFromUrl('  github.com  ')).toBe('GitHub')
    })
  })
})
