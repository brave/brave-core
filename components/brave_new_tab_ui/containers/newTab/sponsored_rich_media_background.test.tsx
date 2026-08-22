/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as BraveAds from 'gen/brave/components/brave_ads/core/mojom/brave_ads.mojom.m.js'

import {
  dispatchRichMediaMessage,
  RichMediaMessageCapabilities,
} from './sponsored_rich_media_background'

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
