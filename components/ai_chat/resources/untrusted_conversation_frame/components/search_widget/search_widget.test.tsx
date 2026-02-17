// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Set up loadTimeData mock BEFORE importing components
;(window as any).loadTimeData = {
  getString: jest.fn((key: string) => {
    if (key === 'apiHost') {
      return 'https://api.example.com'
    }
    return ''
  }),
  getStringF: jest.fn().mockReturnValue(''),
  getInteger: jest.fn().mockReturnValue(0),
  getBoolean: jest.fn().mockReturnValue(false),
  set: jest.fn(),
  data_: {},
}

import * as React from 'react'
import '@testing-library/jest-dom'
import { render, screen, waitFor } from '@testing-library/react'
import SearchWidget from './search_widget'

const mockWebSearchResults = {
  type: 'search',
  query: { original: 'test query' },
  web: {
    results: [
      {
        type: 'search_result',
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
        type: 'search_result',
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
    ],
  },
}

const mockImageResults = {
  type: 'images',
  query: { original: 'test query' },
  results: [
    {
      type: 'image_result',
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
  ],
}

const mockVideoResults = {
  type: 'videos',
  query: { original: 'test query' },
  results: [
    {
      type: 'video_result',
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
  ],
}

const mockNewsResults = {
  type: 'news',
  query: { original: 'test query' },
  results: [
    {
      type: 'news_result',
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
  ],
}

describe('SearchWidget', () => {
  let mockFetch: jest.Mock

  beforeEach(() => {
    mockFetch = jest.fn((url) => {
      const urlString = url.toString()
      if (urlString.includes('/web/')) {
        return Promise.resolve({
          json: () => Promise.resolve(mockWebSearchResults),
        } as Response)
      } else if (urlString.includes('/images/')) {
        return Promise.resolve({
          json: () => Promise.resolve(mockImageResults),
        } as Response)
      } else if (urlString.includes('/videos/')) {
        return Promise.resolve({
          json: () => Promise.resolve(mockVideoResults),
        } as Response)
      } else if (urlString.includes('/news/')) {
        return Promise.resolve({
          json: () => Promise.resolve(mockNewsResults),
        } as Response)
      }
      return Promise.reject(new Error('Unknown URL'))
    })
    global.fetch = mockFetch
  })

  afterEach(() => {
    jest.restoreAllMocks()
  })

  test('should render loading state initially', async () => {
    const { container } = render(
      <SearchWidget
        query='test query'
        type='web'
      />,
    )
    // Check for the leo-progressring custom element
    const progressRing = container.querySelector('leo-progressring')
    expect(progressRing).toBeInTheDocument()

    // Wait for loading to complete to avoid act warnings
    await waitFor(() => {
      expect(screen.getByText('Test Result 1')).toBeInTheDocument()
    })
  })

  test('should render web search results', async () => {
    render(
      <SearchWidget
        query='test query'
        type='web'
      />,
    )

    await waitFor(() => {
      expect(screen.getByText('Test Result 1')).toBeInTheDocument()
    })

    expect(screen.getByText('Test Result 2')).toBeInTheDocument()
    const links = screen.getAllByRole('link')
    expect(links.length).toBeGreaterThanOrEqual(2) // At least 2 results
  })

  test('should render image results', async () => {
    render(
      <SearchWidget
        query='test query'
        type='images'
      />,
    )

    // Wait for results to load
    await waitFor(() => {
      const links = screen.getAllByRole('link')
      expect(links.length).toBeGreaterThan(0)
    })

    // Should fetch images endpoint
    await waitFor(() => {
      expect(mockFetch).toHaveBeenCalledWith(
        expect.stringContaining('/images/search'),
      )
    })
  })

  test('should render video results with age', async () => {
    render(
      <SearchWidget
        query='test query'
        type='videos'
      />,
    )

    await waitFor(() => {
      expect(screen.getByText('Test Video 1')).toBeInTheDocument()
    })

    expect(screen.getByText('2 days ago')).toBeInTheDocument()
  })

  test('should render news results with age', async () => {
    render(
      <SearchWidget
        query='test query'
        type='news'
      />,
    )

    await waitFor(() => {
      expect(screen.getByText('Test News 1')).toBeInTheDocument()
    })

    expect(screen.getByText('1 hour ago')).toBeInTheDocument()
  })

  test('should render type selector buttons', async () => {
    const { container } = render(
      <SearchWidget
        query='test query'
        type='web'
      />,
    )

    // Wait for initial web results
    await waitFor(() => {
      expect(screen.getByText('Test Result 1')).toBeInTheDocument()
    })

    // Should have 4 type selector buttons (web, images, videos, news)
    const typeButtons = container.querySelectorAll('.types leo-button')
    expect(typeButtons).toHaveLength(4)

    // The first button (web) should have 'plain' kind (active state)
    expect(typeButtons[0].getAttribute('kind')).toBe('plain')
  })

  test('should render description with strong tags', async () => {
    const { container } = render(
      <SearchWidget
        query='test query'
        type='web'
      />,
    )

    await waitFor(() => {
      expect(screen.getByText('Test Result 1')).toBeInTheDocument()
    })

    // The description should be present (strong tags are handled specially)
    const descriptions = container.querySelectorAll('.description')
    expect(descriptions.length).toBeGreaterThan(0)
    expect(descriptions[0].textContent).toContain('description')
  })

  test('should have carousel buttons', async () => {
    const { container } = render(
      <SearchWidget
        query='test query'
        type='web'
      />,
    )

    await waitFor(() => {
      expect(screen.getByText('Test Result 1')).toBeInTheDocument()
    })

    const carouselButtons = container.querySelectorAll('.carouselButton')
    expect(carouselButtons).toHaveLength(2)
  })

  test('should disable left carousel button at start', async () => {
    const { container } = render(
      <SearchWidget
        query='test query'
        type='web'
      />,
    )

    await waitFor(() => {
      expect(screen.getByText('Test Result 1')).toBeInTheDocument()
    })

    const carouselButtons = container.querySelectorAll('.carouselButton')
    const leftButton = carouselButtons[0] // First carousel button is left

    // Left button should be disabled at the start
    expect(leftButton.getAttribute('isdisabled')).toBe('true')
  })

  test('should render search query link', async () => {
    render(
      <SearchWidget
        query='test query'
        type='web'
      />,
    )

    await waitFor(() => {
      expect(screen.getByText('test query')).toBeInTheDocument()
    })

    const queryLink = screen.getByRole('link', { name: /test query/ })
    expect(queryLink).toHaveAttribute(
      'href',
      'https://search.brave.com/search?q=test%20query',
    )
  })

  test('should encode query parameter in API call', async () => {
    render(
      <SearchWidget
        query='test & query'
        type='web'
      />,
    )

    await waitFor(() => {
      expect(mockFetch).toHaveBeenCalledWith(
        expect.stringContaining('q=test%20%26%20query'),
      )
    })
  })

  test('should handle empty results gracefully', async () => {
    mockFetch.mockImplementation(() =>
      Promise.resolve({
        json: () =>
          Promise.resolve({
            type: 'search',
            query: { original: 'test' },
            web: { results: [] },
          }),
      } as Response),
    )

    const { container } = render(
      <SearchWidget
        query='test query'
        type='web'
      />,
    )

    await waitFor(() => {
      expect(
        container.querySelector('leo-progressring'),
      ).not.toBeInTheDocument()
    })

    // Should not crash with empty results
    const links = screen.getAllByRole('link')
    expect(links.length).toBe(1) // Only the query link
  })
})
