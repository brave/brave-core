// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Set up loadTimeData mock BEFORE importing components
;(window as any).loadTimeData = {
  getString: jest.fn().mockReturnValue(''),
  getStringF: jest.fn().mockReturnValue(''),
  getInteger: jest.fn().mockReturnValue(0),
  getBoolean: jest.fn().mockReturnValue(false),
  set: jest.fn(),
  data_: {},
}

import * as React from 'react'
import '@testing-library/jest-dom'
import { render, screen, fireEvent } from '@testing-library/react'
import SearchWidget from './search_widget'

const webResults = [
  {
    type: 'search_result' as const,
    title: 'Test Result 1',
    url: 'https://example.com/1',
    description: 'Test <strong>description</strong> 1',
    thumbnail: { src: 'https://example.com/thumb1.jpg' },
    meta_url: {
      favicon: 'https://example.com/favicon1.ico',
      path: '/path1',
      scheme: 'https',
      hostname: 'example.com',
      net_loc: 'example.com',
    },
  },
  {
    type: 'search_result' as const,
    title: 'Test Result 2',
    url: 'https://example.com/2',
    description: 'Test description 2',
    thumbnail: { src: 'https://example.com/thumb2.jpg' },
    meta_url: {
      favicon: 'https://example.com/favicon2.ico',
      path: '/path2',
      scheme: 'https',
      hostname: 'example.com',
      net_loc: 'example.com',
    },
  },
]

const imageResults = [
  {
    type: 'image_result' as const,
    title: 'Test Image 1',
    url: 'https://example.com/image1',
    description: '',
    thumbnail: {
      src: 'https://example.com/thumb1.jpg',
      original: 'https://example.com/orig1.jpg',
    },
    meta_url: {
      favicon: 'https://example.com/favicon.ico',
      path: '',
      scheme: 'https',
      hostname: 'example.com',
      net_loc: 'example.com',
    },
  },
]

const videoResults = [
  {
    type: 'video_result' as const,
    title: 'Test Video 1',
    url: 'https://example.com/video1',
    description: 'Video description',
    thumbnail: { src: 'https://example.com/video-thumb1.jpg' },
    age: '2 days ago',
    meta_url: {
      favicon: 'https://example.com/favicon.ico',
      path: '',
      scheme: 'https',
      hostname: 'example.com',
      net_loc: 'example.com',
    },
  },
]

const newsResults = [
  {
    type: 'news_result' as const,
    title: 'Test News 1',
    url: 'https://example.com/news1',
    description: 'News description',
    thumbnail: { src: 'https://example.com/news-thumb1.jpg' },
    age: '1 hour ago',
    meta_url: {
      favicon: 'https://example.com/favicon.ico',
      path: '',
      scheme: 'https',
      hostname: 'example.com',
      net_loc: 'example.com',
    },
  },
]

describe('SearchWidget', () => {
  test('should render web search results', () => {
    render(
      <SearchWidget
        query='test query'
        type='web'
        results={webResults}
      />,
    )

    expect(screen.getByText('Test Result 1')).toBeInTheDocument()
    expect(screen.getByText('Test Result 2')).toBeInTheDocument()
    const links = screen.getAllByRole('link')
    expect(links.length).toBeGreaterThanOrEqual(2)
  })

  test('should render image results', () => {
    render(
      <SearchWidget
        query='test query'
        type='images'
        results={imageResults}
      />,
    )

    const links = screen.getAllByRole('link')
    expect(links.length).toBeGreaterThan(0)
  })

  test('should render video results with age', () => {
    render(
      <SearchWidget
        query='test query'
        type='videos'
        results={videoResults}
      />,
    )

    expect(screen.getByText('Test Video 1')).toBeInTheDocument()
    expect(screen.getByText('2 days ago')).toBeInTheDocument()
  })

  test('should render news results with age', () => {
    render(
      <SearchWidget
        query='test query'
        type='news'
        results={newsResults}
      />,
    )

    expect(screen.getByText('Test News 1')).toBeInTheDocument()
    expect(screen.getByText('1 hour ago')).toBeInTheDocument()
  })

  test('should render type selector buttons for available result types', () => {
    const { container } = render(
      <SearchWidget
        query='test query'
        type='web'
        results={[
          ...webResults,
          ...imageResults,
          ...videoResults,
          ...newsResults,
        ]}
      />,
    )

    // Should have 4 type selector buttons (web, images, videos, news)
    const typeButtons = container.querySelectorAll('.types leo-button')
    expect(typeButtons).toHaveLength(4)

    // The first button (web) should have 'plain' kind (active state)
    expect(typeButtons[0].getAttribute('kind')).toBe('plain')
  })

  test('should render description with strong tags', () => {
    const { container } = render(
      <SearchWidget
        query='test query'
        type='web'
        results={webResults}
      />,
    )

    const descriptions = container.querySelectorAll('.description')
    expect(descriptions.length).toBeGreaterThan(0)
    expect(descriptions[0].textContent).toContain('description')
  })

  test('should have carousel buttons', () => {
    const { container } = render(
      <SearchWidget
        query='test query'
        type='web'
        results={webResults}
      />,
    )

    const carouselButtons = container.querySelectorAll('.carouselButton')
    expect(carouselButtons).toHaveLength(2)
  })

  test('should disable left carousel button at start', () => {
    const { container } = render(
      <SearchWidget
        query='test query'
        type='web'
        results={webResults}
      />,
    )

    const carouselButtons = container.querySelectorAll('.carouselButton')
    const leftButton = carouselButtons[0] // First carousel button is left

    // Left button should be disabled at the start
    expect(leftButton.getAttribute('isdisabled')).toBe('true')
  })

  test('should render search query link', () => {
    render(
      <SearchWidget
        query='test query'
        type='web'
        results={webResults}
      />,
    )

    expect(screen.getByText('test query')).toBeInTheDocument()

    const queryLink = screen.getByRole('link', { name: /test query/ })
    expect(queryLink).toHaveAttribute(
      'href',
      'https://search.brave.com/search?q=test%20query',
    )
  })

  test('should encode query parameter in search link', () => {
    render(
      <SearchWidget
        query='test & query'
        type='web'
        results={webResults}
      />,
    )

    const queryLink = screen.getByRole('link', { name: /test & query/ })
    expect(queryLink).toHaveAttribute(
      'href',
      expect.stringContaining('q=test%20%26%20query'),
    )
  })

  test('should fall back to first type with results when specified type has no results', () => {
    // type='web' but only image results provided - should fall back to 'images'
    render(
      <SearchWidget
        query='test query'
        type='web'
        results={imageResults}
      />,
    )

    // The query footer link reflects the active type - should be 'images' after fallback
    const queryLink = screen.getByRole('link', { name: /test query/ })
    expect(queryLink).toHaveAttribute(
      'href',
      'https://search.brave.com/images?q=test%20query',
    )
  })

  test('should fall back to earliest available type (per searchTypes order) when specified type has no results', () => {
    // type='news' but only web and video results provided - should fall back to 'web' (first in order)
    render(
      <SearchWidget
        query='test query'
        type='news'
        results={[...webResults, ...videoResults]}
      />,
    )

    expect(screen.getByText('Test Result 1')).toBeInTheDocument()
    const queryLink = screen.getByRole('link', { name: /test query/ })
    expect(queryLink).toHaveAttribute(
      'href',
      'https://search.brave.com/search?q=test%20query',
    )
  })

  test('should not fall back when specified type has results', () => {
    // type='images' with both web and image results - should stay on 'images'
    render(
      <SearchWidget
        query='test query'
        type='images'
        results={[...webResults, ...imageResults]}
      />,
    )

    const queryLink = screen.getByRole('link', { name: /test query/ })
    expect(queryLink).toHaveAttribute(
      'href',
      'https://search.brave.com/images?q=test%20query',
    )
  })

  describe('image error handling', () => {
    test('hides entire card when an image result thumbnail fails', () => {
      const { container } = render(
        <SearchWidget
          query='test query'
          type='images'
          results={imageResults}
        />,
      )

      const thumbnail = container.querySelector('.thumbnail')
      expect(thumbnail).toBeInTheDocument()

      fireEvent.error(thumbnail!)

      expect(container.querySelector('.imageResult')).not.toBeInTheDocument()
    })

    test('hides only thumbnail with visibility:hidden for video when image fails', () => {
      const { container } = render(
        <SearchWidget
          query='test query'
          type='videos'
          results={videoResults}
        />,
      )

      const thumbnail = container.querySelector('.thumbnail')
      fireEvent.error(thumbnail!)

      expect(screen.getByText('Test Video 1')).toBeInTheDocument()
      expect(thumbnail).toHaveStyle({ visibility: 'hidden' })
    })

    test('hides only thumbnail with visibility:hidden for news when image fails', () => {
      const { container } = render(
        <SearchWidget
          query='test query'
          type='news'
          results={newsResults}
        />,
      )

      const thumbnail = container.querySelector('.thumbnail')
      fireEvent.error(thumbnail!)

      expect(screen.getByText('Test News 1')).toBeInTheDocument()
      expect(thumbnail).toHaveStyle({ visibility: 'hidden' })
    })
  })

  test('should render placeholder for empty results', () => {
    const { container } = render(
      <SearchWidget
        query='test query'
        type='web'
        results={[]}
      />,
    )

    expect(container.querySelector('.placeholderResult')).toBeInTheDocument()
    expect(
      screen.queryAllByRole('link').filter((l) => l.closest('.searchResults')),
    ).toHaveLength(0)
  })
})
