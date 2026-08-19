/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { act, fireEvent, render, screen } from '@testing-library/react'
import { SponsoredSite } from '../../state/top_sites_store'
import { SponsoredSitesTile } from './sponsored_site_tile'

// The default $web-common/locale mock (see components/test/testSetup.ts)
// echoes the string key back verbatim, which has no $1/$2 placeholders for
// formatString to replace. Override it here with a string that has both, so
// the tooltip renders as it would in production.
jest.mock('$web-common/locale', () => ({
  getLocale: () => 'Sponsored by $1. $2Learn more/$2.',
}))

// The tooltip content also renders a "Learn more" <a>, so getByRole('link')
// alone is ambiguous. This scopes to the tile's own anchor specifically.
function getTileLink() {
  return document.querySelector<HTMLAnchorElement>('a.top-site-tile')!
}

// The Leo tooltip web component receives its props as plain JS properties
// set directly on the element, not as HTML attributes.
function getTooltipElement() {
  return document.querySelector('leo-tooltip') as
    | (HTMLElement & { visible?: boolean; mouseleaveTimeout?: number })
    | null
}

function getAdDisclosure() {
  return screen.getByText('bar')
}

function createSite(overrides: Partial<SponsoredSite> = {}): SponsoredSite {
  return {
    relativeImageUrl: 'chrome://branded-wallpaper/sponsored-images/foo',
    title: 'foo',
    adDisclosure: 'bar',
    targetUrl: 'https://foo.com',
    ...overrides,
  }
}

describe('SponsoredSitesTile', () => {
  it('should render the tile title, image, and ad disclosure', () => {
    const site = createSite()
    render(
      <SponsoredSitesTile
        site={site}
        onContextMenu={() => {}}
      />,
    )
    expect(screen.getByText('foo')).toBeInTheDocument()
    expect(screen.getByText('bar')).toBeInTheDocument()
    expect(screen.getByRole('img')).toHaveAttribute(
      'src',
      'chrome://branded-wallpaper/sponsored-images/foo',
    )
  })

  it('should give a grace period before hiding on mouse leave, so a pointer moving toward "Learn more" across the gap between the tile and the tooltip is not cut off', () => {
    render(
      <SponsoredSitesTile
        site={createSite()}
        onContextMenu={() => {}}
      />,
    )
    expect(getTooltipElement()?.mouseleaveTimeout).toBe(500)
  })

  it('should omit the ad disclosure when the site does not have one', () => {
    const { container } = render(
      <SponsoredSitesTile
        site={createSite({ adDisclosure: '' })}
        onContextMenu={() => {}}
      />,
    )
    expect(
      container.querySelector('.top-site-ad-disclosure'),
    ).not.toBeInTheDocument()
  })

  it.each([
    [
      'a valid https target URL',
      'https://foo.com/path',
      'https://foo.com/path',
    ],
    ['a disallowed javascript: scheme', 'javascript:alert(1)', ''],
    [
      'a disallowed data: scheme',
      'data:text/html,<script>alert(1)</script>',
      '',
    ],
    ['a disallowed non-https scheme', 'http://foo.com', ''],
  ])(
    'should use %s as the tile href',
    (_description: string, targetUrl: string, expectedHref: string) => {
      render(
        <SponsoredSitesTile
          site={createSite({ targetUrl })}
          onContextMenu={() => {}}
        />,
      )
      expect(getTileLink()).toHaveAttribute('href', expectedHref)
    },
  )

  describe('tooltip visibility', () => {
    beforeEach(() => {
      jest.useFakeTimers()
    })

    afterEach(() => {
      jest.useRealTimers()
    })

    it('should not show the tooltip immediately on hover', () => {
      render(
        <SponsoredSitesTile
          site={createSite()}
          onContextMenu={() => {}}
        />,
      )
      fireEvent.mouseEnter(getAdDisclosure())
      expect(getTooltipElement()?.visible).toBe(false)
    })

    it('should show the tooltip after a delay on hover', () => {
      render(
        <SponsoredSitesTile
          site={createSite()}
          onContextMenu={() => {}}
        />,
      )
      fireEvent.mouseEnter(getAdDisclosure())
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(true)
    })

    it('should not show the tooltip if the pointer leaves before the delay elapses', () => {
      render(
        <SponsoredSitesTile
          site={createSite()}
          onContextMenu={() => {}}
        />,
      )
      fireEvent.mouseEnter(getAdDisclosure())
      act(() => {
        jest.advanceTimersByTime(300)
      })
      fireEvent.mouseLeave(getAdDisclosure())
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(false)
    })

    it('should show the tooltip immediately on focus', () => {
      render(
        <SponsoredSitesTile
          site={createSite()}
          onContextMenu={() => {}}
        />,
      )
      fireEvent.focus(getAdDisclosure())
      expect(getTooltipElement()?.visible).toBe(true)
    })

    it('should keep the tooltip open when focus moves to the learn more link', () => {
      render(
        <SponsoredSitesTile
          site={createSite()}
          onContextMenu={() => {}}
        />,
      )
      // Uses real .focus() calls, not fireEvent with a fake relatedTarget,
      // since only a real focus transfer updates document.activeElement
      // and gives the resulting blur event a genuine relatedTarget.
      act(() => getAdDisclosure().focus())
      act(() => screen.getByRole('link', { name: 'Learn more' }).focus())
      expect(getTooltipElement()?.visible).toBe(true)
    })

    it('should close the tooltip when focus moves away from it entirely', () => {
      render(
        <SponsoredSitesTile
          site={createSite()}
          onContextMenu={() => {}}
        />,
      )
      act(() => getAdDisclosure().focus())
      const elsewhere = document.createElement('button')
      document.body.appendChild(elsewhere)
      act(() => elsewhere.focus())
      expect(getTooltipElement()?.visible).toBe(false)
    })

    it('should not show the tooltip when hovering the icon or title', () => {
      render(
        <SponsoredSitesTile
          site={createSite()}
          onContextMenu={() => {}}
        />,
      )
      fireEvent.mouseEnter(screen.getByText('foo'))
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(false)
    })
  })

  it('should not be draggable', () => {
    render(
      <SponsoredSitesTile
        site={createSite()}
        onContextMenu={() => {}}
      />,
    )
    expect(getTileLink()).toHaveAttribute('draggable', 'false')
  })

  it('should call onContextMenu when the tile is right-clicked', () => {
    const onContextMenu = jest.fn()
    render(
      <SponsoredSitesTile
        site={createSite()}
        onContextMenu={onContextMenu}
      />,
    )
    getTileLink().dispatchEvent(
      new MouseEvent('contextmenu', { bubbles: true, cancelable: true }),
    )
    expect(onContextMenu).toHaveBeenCalledTimes(1)
  })
})
