// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'
import {
  LongVisualContentWarning,
  LongTextContentWarning,
  LongPageContentWarning,
} from './long_page_info'

describe('Long Page Warning Components', () => {
  beforeEach(() => {
    // Mock the global S object
    global.S = {
      CHAT_UI_VISUAL_CONTENT_TOO_MUCH_WARNING:
        'CHAT_UI_VISUAL_CONTENT_TOO_MUCH_WARNING',
      CHAT_UI_TRIMMED_TOKENS_WARNING: 'CHAT_UI_TRIMMED_TOKENS_WARNING',
      CHAT_UI_PAGE_CONTENT_TOO_LONG_WARNING:
        'CHAT_UI_PAGE_CONTENT_TOO_LONG_WARNING',
    }
  })

  afterEach(() => {
    jest.restoreAllMocks()
  })

  describe('LongVisualContentWarning', () => {
    test('should display visual content warning with 0%', () => {
      const { getByText } = render(
        <LongVisualContentWarning visualContentUsedPercentage={0} />,
      )

      expect(
        getByText('CHAT_UI_VISUAL_CONTENT_TOO_MUCH_WARNING'),
      ).toBeInTheDocument()
    })

    test('should display visual content warning with 99%', () => {
      const { getByText } = render(
        <LongVisualContentWarning visualContentUsedPercentage={99} />,
      )

      expect(
        getByText('CHAT_UI_VISUAL_CONTENT_TOO_MUCH_WARNING'),
      ).toBeInTheDocument()
    })
  })

  describe('LongTextContentWarning', () => {
    test('should display text content warning', () => {
      const { getByText } = render(
        <LongTextContentWarning percentageUsed={90} />,
      )

      expect(getByText('CHAT_UI_TRIMMED_TOKENS_WARNING')).toBeInTheDocument()
    })

    test('should display text content warning with low percentage', () => {
      const { getByText } = render(
        <LongTextContentWarning percentageUsed={25} />,
      )

      expect(getByText('CHAT_UI_TRIMMED_TOKENS_WARNING')).toBeInTheDocument()
    })
  })

  describe('LongPageContentWarning', () => {
    test('should display page content warning with correct percentage', () => {
      const { getByText } = render(
        <LongPageContentWarning contentUsedPercentage={85} />,
      )

      expect(
        getByText('CHAT_UI_PAGE_CONTENT_TOO_LONG_WARNING'),
      ).toBeInTheDocument()
    })

    test('should display page content warning with low percentage', () => {
      const { getByText } = render(
        <LongPageContentWarning contentUsedPercentage={50} />,
      )

      expect(
        getByText('CHAT_UI_PAGE_CONTENT_TOO_LONG_WARNING'),
      ).toBeInTheDocument()
    })
  })

  describe('component structure', () => {
    test('should render with info icon', () => {
      const { container } = render(
        <LongVisualContentWarning visualContentUsedPercentage={80} />,
      )

      // Check for icon (leo-icon with name 'info-outline')
      const icon = container.querySelector('leo-icon[name="info-outline"]')
      expect(icon).toBeInTheDocument()
    })

    test('should have correct CSS class structure', () => {
      const { container } = render(
        <LongPageContentWarning contentUsedPercentage={75} />,
      )

      // Check for info container with CSS class
      const infoDiv = container.querySelector('div[class*="info"]')
      expect(infoDiv).toBeInTheDocument()
    })
  })
})
