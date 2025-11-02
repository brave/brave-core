/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render, screen } from '@testing-library/react'
import GeneratedFilterDisplay, { GeneratedFilterData } from './index'

describe('GeneratedFilterDisplay', () => {
  const mockCSSFilter: GeneratedFilterData = {
    filterType: 'css_selector',
    domain: 'example.com',
    code: 'example.com##.cookie-banner',
    description: 'Hides the cookie consent banner',
    targetElements: ['.cookie-banner'],
    confidence: 'high',
    reasoning: 'The class name clearly indicates this is a cookie banner',
  }

  const mockScriptletFilter: GeneratedFilterData = {
    filterType: 'scriptlet',
    domain: 'example.com',
    code: `(() => {
  console.log('[Leo Scriptlet] Starting execution');
  document.querySelectorAll('[data-ad]').forEach(el => el.remove());
  console.log('[Leo Scriptlet] Execution complete');
})()`,
    description: 'Removes all elements with data-ad attribute',
    targetElements: ['[data-ad]'],
    confidence: 'medium',
    reasoning: 'Using scriptlet to completely remove elements instead of just hiding them',
  }

  it('renders CSS filter correctly', () => {
    render(<GeneratedFilterDisplay filter={mockCSSFilter} />)

    expect(screen.getByText('Cosmetic Filter')).toBeInTheDocument()
    expect(screen.getByText('Hides the cookie consent banner')).toBeInTheDocument()
    expect(screen.getByText('.cookie-banner')).toBeInTheDocument()
    expect(screen.getByText('High confidence')).toBeInTheDocument()
    expect(screen.getByText(/class name clearly indicates/i)).toBeInTheDocument()
    expect(screen.getByText(/This filter will apply to:/i)).toBeInTheDocument()
    expect(screen.getByText('example.com')).toBeInTheDocument()
  })

  it('renders scriptlet filter correctly', () => {
    render(<GeneratedFilterDisplay filter={mockScriptletFilter} />)

    expect(screen.getByText('Custom Scriptlet')).toBeInTheDocument()
    expect(screen.getByText('Removes all elements with data-ad attribute')).toBeInTheDocument()
    expect(screen.getByText('[data-ad]')).toBeInTheDocument()
    expect(screen.getByText('Medium confidence')).toBeInTheDocument()
    expect(screen.getByText(/Using scriptlet to completely remove/i)).toBeInTheDocument()
    expect(screen.getByText(/Debug logs will appear in DevTools Console/i)).toBeInTheDocument()
  })

  it('displays low confidence with correct styling', () => {
    const lowConfidenceFilter: GeneratedFilterData = {
      ...mockCSSFilter,
      confidence: 'low',
    }

    render(<GeneratedFilterDisplay filter={lowConfidenceFilter} />)
    expect(screen.getByText('Low confidence')).toBeInTheDocument()
  })

  it('displays multiple target elements', () => {
    const multiTargetFilter: GeneratedFilterData = {
      ...mockCSSFilter,
      targetElements: ['.banner', '#popup', '.cookie-notice'],
    }

    render(<GeneratedFilterDisplay filter={multiTargetFilter} />)
    expect(screen.getByText('.banner')).toBeInTheDocument()
    expect(screen.getByText('#popup')).toBeInTheDocument()
    expect(screen.getByText('.cookie-notice')).toBeInTheDocument()
  })
})
