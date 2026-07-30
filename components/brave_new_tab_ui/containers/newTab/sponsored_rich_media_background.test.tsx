// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'
import * as BraveAds from 'gen/brave/components/brave_ads/core/mojom/brave_ads.mojom.m.js'

import type { RectF } from 'gen/ui/webui/resources/tsc/mojo/ui/gfx/geometry/mojom/geometry.mojom-webui'

import {
  SponsoredRichMediaBackground,
  SponsoredRichMediaBackgroundInfo
} from './sponsored_rich_media_background'

const ntpNewTabTakeoverRichMediaUrl = 'chrome-untrusted://new-tab-takeover/'
const backgroundInfo: SponsoredRichMediaBackgroundInfo = {
  url: ntpNewTabTakeoverRichMediaUrl,
  placementId: 'e1cb0d20-8b6e-4b1e-9c1e-1e6ff5b2f8e7',
  creativeInstanceId: '7f0e2f1d-9c3a-4f2b-8a1d-2e5c7b9f4a3d',
  metricType: BraveAds.NewTabPageAdMetricType.kConfirmation,
  targetUrl: 'https://brave.com'
}

jest.mock('$web-common/loadTimeData', () => ({
  loadTimeData: {
    getString: (key: string) =>
      key === 'ntpNewTabTakeoverRichMediaUrl'
        ? 'chrome-untrusted://new-tab-takeover/'
        : key
  }
}))

let postMessage: jest.Mock
beforeEach(() => {
  postMessage = jest.fn()
  jest
    .spyOn(HTMLIFrameElement.prototype, 'contentWindow', 'get')
    .mockReturnValue({ postMessage })
})
afterEach(() => {
  jest.restoreAllMocks()
})

function sponsoredRichMediaBackground(
  safeArea?: RectF,
  richMediaHasLoaded = true
) {
  return (
    <SponsoredRichMediaBackground
      sponsoredRichMediaBackgroundInfo={backgroundInfo}
      richMediaHasLoaded={richMediaHasLoaded}
      safeArea={safeArea}
      onEventReported={() => {}}
      onLoaded={() => {}}
    />
  )
}

describe('SponsoredRichMediaBackground safe area', () => {
  it('posts safe area', () => {
    render(sponsoredRichMediaBackground({
      x: 0,
      y: 42,
      width: 360,
      height: 200
    }))

    expect(postMessage).toHaveBeenCalledWith(
      {
        type: 'richMediaSafeRect',
        value: { x: 0, y: 42, width: 360, height: 200 }
      },
      expect.any(String)
    )
  })

  it('does not post safe area when undefined', () => {
    render(sponsoredRichMediaBackground(undefined))

    expect(postMessage).not.toHaveBeenCalled()
  })

  it('does not post safe area before rich media has loaded', () => {
    render(sponsoredRichMediaBackground(
      { x: 0, y: 42, width: 360, height: 200 },
      /* richMediaHasLoaded= */ false
    ))

    expect(postMessage).not.toHaveBeenCalled()
  })

  it('posts safe area once rich media has loaded', () => {
    const safeArea = { x: 0, y: 42, width: 360, height: 200 }
    const { rerender } = render(sponsoredRichMediaBackground(
      safeArea,
      /* richMediaHasLoaded= */ false
    ))

    expect(postMessage).not.toHaveBeenCalled()

    rerender(sponsoredRichMediaBackground(
      safeArea,
      /* richMediaHasLoaded= */ true
    ))

    expect(postMessage).toHaveBeenCalledTimes(1)
    expect(postMessage).toHaveBeenCalledWith(
      {
        type: 'richMediaSafeRect',
        value: safeArea
      },
      expect.any(String)
    )
  })

  it('reposts safe area when it changes', () => {
    const { rerender } = render(sponsoredRichMediaBackground({
      x: 0,
      y: 42,
      width: 360,
      height: 200
    }))

    rerender(sponsoredRichMediaBackground({
      x: 0,
      y: 42,
      width: 360,
      height: 120
    }))

    expect(postMessage).toHaveBeenCalledTimes(2)
    expect(postMessage).toHaveBeenLastCalledWith(
      {
        type: 'richMediaSafeRect',
        value: { x: 0, y: 42, width: 360, height: 120 }
      },
      expect.any(String)
    )
  })
})
