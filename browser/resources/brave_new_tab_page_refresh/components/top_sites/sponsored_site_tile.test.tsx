/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render, screen } from '@testing-library/react'
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
