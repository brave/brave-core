/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Brands where title-casing the domain label produces the wrong result.
const brandCapitalization: Record<string, string> = {
  github: 'GitHub',
  youtube: 'YouTube',
  linkedin: 'LinkedIn',
  openai: 'OpenAI',
  leetcode: 'LeetCode',
  ycombinator: 'Y Combinator',
}

// Second-level TLD suffixes that require stripping two labels (e.g. co.uk).
const multiPartTlds = new Set([
  'co.in', 'co.uk', 'co.jp', 'co.nz', 'co.za', 'co.kr',
  'com.au', 'com.br',
  'org.uk',
])

// Single-label TLDs whose only purpose is to identify the TLD boundary.
const singleTlds = new Set([
  'com', 'org', 'net', 'io', 'dev', 'ai',
  'in', 'uk', 'us', 'ca', 'de', 'fr', 'jp', 'au', 'br',
  'edu', 'gov', 'me', 'app', 'co',
])

function parseInputUrl(input: string): URL | null {
  if (!input) {
    return null
  }
  // The URL spec allows bare words as scheme names (e.g. "localhost:3000"
  // parses as scheme "localhost:" with an empty hostname). Only accept a
  // parsed URL when it actually produced a hostname.
  try {
    const url = new URL(input)
    if (url.hostname) {
      return url
    }
  } catch {
    // Fall through.
  }
  try {
    return new URL(`https://${input}`)
  } catch {
    return null
  }
}

function isIPv4Host(host: string) {
  return /^\d{1,3}(?:\.\d{1,3}){3}$/.test(host)
}

function titleCaseWords(value: string) {
  return value
    .split(/\s+/)
    .filter(Boolean)
    .map((word) => word[0].toUpperCase() + word.slice(1))
    .join(' ')
}

function removeKnownTld(labels: string[]) {
  if (labels.length < 2) {
    return labels
  }
  const lastTwo = labels.slice(-2).join('.')
  if (multiPartTlds.has(lastTwo)) {
    return labels.slice(0, -2)
  }
  if (singleTlds.has(labels[labels.length - 1])) {
    return labels.slice(0, -1)
  }
  return labels
}

export function generateSiteNameFromUrl(input: string): string {
  const url = parseInputUrl(input.trim())
  if (!url) {
    return ''
  }

  const host = url.hostname.toLowerCase().replace(/^www\./, '')
  if (!host) {
    return ''
  }

  if (host === 'localhost') {
    return 'Localhost'
  }

  // IPv6 addresses contain colons; return the raw host for both IPv4 and IPv6.
  if (isIPv4Host(host) || host.includes(':')) {
    return host
  }

  let labels = host.split('.').filter(Boolean)
  if (labels.length === 0) {
    return ''
  }

  labels = removeKnownTld(labels)
  if (labels.length === 0) {
    return ''
  }

  const baseLabel = labels[labels.length - 1]
  const brandName = brandCapitalization[baseLabel]
  if (brandName) {
    return brandName
  }

  return titleCaseWords(baseLabel.replace(/[-_]+/g, ' '))
}
