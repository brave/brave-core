/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * PREVIEW COMPONENT - FOR TESTING ONLY
 *
 * This file demonstrates how the GeneratedFilterDisplay component looks
 * with different types of filters. To use it:
 *
 * 1. Temporarily import this in conversation_entries/index.tsx:
 *    import GeneratedFilterPreview from '../generated_filter_display/preview'
 *
 * 2. Add it to the render output:
 *    <GeneratedFilterPreview />
 *
 * 3. Launch the browser and open Leo to see the previews
 *
 * Remember to remove this before committing!
 */

import * as React from 'react'
import GeneratedFilterDisplay, { GeneratedFilterData } from './index'

const exampleFilters: GeneratedFilterData[] = [
  // Example 1: High confidence CSS filter
  {
    filterType: 'css_selector',
    domain: 'reddit.com',
    code: 'reddit.com##.cookie-banner',
    description: 'Hides the cookie consent banner',
    targetElements: ['.cookie-banner'],
    confidence: 'high',
    reasoning: 'The class name clearly indicates this is a cookie banner. This is a common pattern on Reddit.',
  },

  // Example 2: Medium confidence scriptlet
  {
    filterType: 'scriptlet',
    domain: 'news.ycombinator.com',
    code: `(() => {
  console.log('[Leo Scriptlet] Starting execution');
  document.querySelectorAll('[data-ad]').forEach(el => el.remove());
  console.log('[Leo Scriptlet] Execution complete');
})()`,
    description: 'Removes all advertising elements',
    targetElements: ['[data-ad]', '.sponsored-content'],
    confidence: 'medium',
    reasoning: 'Using scriptlet to completely remove elements instead of just hiding them. The data-ad attribute is commonly used for ads.',
  },

  // Example 3: Low confidence CSS filter with multiple targets
  {
    filterType: 'css_selector',
    domain: 'example.com',
    code: 'example.com##.modal, #popup-overlay',
    description: 'Hides modal popups and overlay',
    targetElements: ['.modal', '#popup-overlay', '.popup-container'],
    confidence: 'low',
    reasoning: 'These selectors are generic and might affect legitimate UI elements. Manual verification recommended.',
  },

  // Example 4: Complex scriptlet
  {
    filterType: 'scriptlet',
    domain: 'github.com',
    code: `(() => {
  console.log('[Leo Scriptlet] Starting execution');

  // Wait for page to fully load
  const removeNotifications = () => {
    const notifications = document.querySelectorAll('.notification-banner');
    notifications.forEach(el => {
      if (el.textContent.includes('promotional')) {
        el.remove();
      }
    });
  };

  // Run immediately and also watch for changes
  removeNotifications();
  new MutationObserver(removeNotifications).observe(document.body, {
    childList: true,
    subtree: true
  });

  console.log('[Leo Scriptlet] Execution complete');
})()`,
    description: 'Removes promotional notification banners',
    targetElements: ['.notification-banner'],
    confidence: 'high',
    reasoning: 'Uses MutationObserver to handle dynamically-loaded promotional notifications. Only removes notifications containing the word "promotional" to avoid removing important alerts.',
  },
]

export default function GeneratedFilterPreview() {
  return (
    <div style={{ padding: '20px', background: 'var(--leo-color-page-background)' }}>
      <h1 style={{ marginBottom: '20px', color: 'var(--leo-color-text-primary)' }}>
        Generated Filter Display Previews
      </h1>
      <p style={{ marginBottom: '30px', color: 'var(--leo-color-text-secondary)' }}>
        These are example filters showing different confidence levels, types, and complexity.
      </p>

      {exampleFilters.map((filter, index) => (
        <GeneratedFilterDisplay key={index} filter={filter} />
      ))}
    </div>
  )
}
