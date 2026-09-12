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
  SponsoredRichMediaBackgroundInfo,
  dispatchRichMediaMessage,
  RichMediaMessageCapabilities,
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

describe('dispatchRichMediaMessage', () => {
  let capabilities: RichMediaMessageCapabilities

  beforeEach(() => {
    capabilities = {
      onEventReported: jest.fn(),
      onAdEventReported: jest.fn(),
      onOpenBraveSearch: jest.fn(),
      onQueryAutocomplete: jest.fn(),
      onMakeBraveSearchDefault: jest.fn(),
    }
  })

  it('should ignore null or undefined data', () => {
    dispatchRichMediaMessage(null, capabilities)
    dispatchRichMediaMessage(undefined, capabilities)

    expect(capabilities.onEventReported).not.toHaveBeenCalled()
  })

  it('should ignore unknown message types', () => {
    dispatchRichMediaMessage({ type: 'unknownType', value: 'click' }, capabilities)

    expect(capabilities.onEventReported).not.toHaveBeenCalled()
    expect(capabilities.onAdEventReported).not.toHaveBeenCalled()
    expect(capabilities.onOpenBraveSearch).not.toHaveBeenCalled()
    expect(capabilities.onQueryAutocomplete).not.toHaveBeenCalled()
    expect(capabilities.onMakeBraveSearchDefault).not.toHaveBeenCalled()
  })

  describe('richMediaEvent', () => {
    it('should report a click event', () => {
      dispatchRichMediaMessage(
        { type: 'richMediaEvent', value: 'click' },
        capabilities,
      )

      expect(capabilities.onEventReported).toHaveBeenCalledWith(
        BraveAds.NewTabPageAdEventType.kClicked,
      )
    })

    it('should report an interaction event', () => {
      dispatchRichMediaMessage(
        { type: 'richMediaEvent', value: 'interaction' },
        capabilities,
      )

      expect(capabilities.onEventReported).toHaveBeenCalledWith(
        BraveAds.NewTabPageAdEventType.kInteraction,
      )
    })

    it('should ignore an unrecognized event value', () => {
      dispatchRichMediaMessage(
        { type: 'richMediaEvent', value: 'notAnEvent' },
        capabilities,
      )

      expect(capabilities.onEventReported).not.toHaveBeenCalled()
    })
  })

  describe('richMediaOpenBraveSearchWithQuery', () => {
    it('should call onOpenBraveSearch with the query value', () => {
      dispatchRichMediaMessage(
        { type: 'richMediaOpenBraveSearchWithQuery', value: 'weather' },
        capabilities,
      )

      expect(capabilities.onOpenBraveSearch).toHaveBeenCalledWith('weather')
    })

    it('should report a click event without navigating to the ad destination', () => {
      dispatchRichMediaMessage(
        { type: 'richMediaOpenBraveSearchWithQuery', value: 'weather' },
        capabilities,
      )

      expect(capabilities.onAdEventReported).toHaveBeenCalledWith(
        BraveAds.NewTabPageAdEventType.kClicked,
      )
      expect(capabilities.onEventReported).not.toHaveBeenCalled()
    })

    it('should ignore a missing value', () => {
      dispatchRichMediaMessage(
        { type: 'richMediaOpenBraveSearchWithQuery' },
        capabilities,
      )

      expect(capabilities.onOpenBraveSearch).not.toHaveBeenCalled()
    })
  })

  describe('richMediaQueryBraveSearchAutocomplete', () => {
    it('should call onQueryAutocomplete with the query value', () => {
      dispatchRichMediaMessage(
        { type: 'richMediaQueryBraveSearchAutocomplete', value: 'weath' },
        capabilities,
      )

      expect(capabilities.onQueryAutocomplete).toHaveBeenCalledWith('weath')
    })

    it('should ignore a missing value', () => {
      dispatchRichMediaMessage(
        { type: 'richMediaQueryBraveSearchAutocomplete' },
        capabilities,
      )

      expect(capabilities.onQueryAutocomplete).not.toHaveBeenCalled()
    })

    it('should call onQueryAutocomplete with an empty string to clear matches', () => {
      dispatchRichMediaMessage(
        { type: 'richMediaQueryBraveSearchAutocomplete', value: '' },
        capabilities,
      )

      expect(capabilities.onQueryAutocomplete).toHaveBeenCalledWith('')
    })
  })

  describe('richMediaMakeBraveSearchDefault', () => {
    it('should call onMakeBraveSearchDefault', () => {
      dispatchRichMediaMessage(
        { type: 'richMediaMakeBraveSearchDefault' },
        capabilities,
      )

      expect(capabilities.onMakeBraveSearchDefault).toHaveBeenCalled()
    })
  })

  describe('richMediaHideBraveSearchBox', () => {
    it('should be recognized but have no effect', () => {
      dispatchRichMediaMessage(
        { type: 'richMediaHideBraveSearchBox' },
        capabilities,
      )

      expect(capabilities.onEventReported).not.toHaveBeenCalled()
      expect(capabilities.onAdEventReported).not.toHaveBeenCalled()
      expect(capabilities.onOpenBraveSearch).not.toHaveBeenCalled()
      expect(capabilities.onQueryAutocomplete).not.toHaveBeenCalled()
      expect(capabilities.onMakeBraveSearchDefault).not.toHaveBeenCalled()
    })
  })
})
