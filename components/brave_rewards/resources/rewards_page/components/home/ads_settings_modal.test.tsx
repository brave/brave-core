/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render, screen } from '@testing-library/react'

import { AppContext } from '../../lib/app_context'
import { AdsInfo, defaultAppStore } from '../../lib/app_store'
import { AdsSettingsModal } from './ads_settings_modal'

// Mock `scoped_css` since it doesn't compile under jest and CSS is not tested here.
jest.mock('$web-common/scoped_css', () => ({
  scoped: {
    css: () => ({}),
  },
}))

function createAdsInfo(overrides: Partial<AdsInfo>): AdsInfo {
  return {
    browserUpgradeRequired: false,
    isSupportedRegion: true,
    adsEnabled: {
      'new-tab-page': true,
      'notification': true,
    },
    adsManagedByPolicy: {
      'new-tab-page': false,
      'notification': false,
    },
    adTypesReceivedThisMonth: {
      'new-tab-page': 1,
      'notification': 2,
    },
    minEarningsPreviousMonth: 0,
    nextPaymentDate: 0,
    notificationAdsPerHour: 5,
    shouldAllowSubdivisionTargeting: false,
    currentSubdivision: '',
    availableSubdivisions: [],
    autoDetectedSubdivision: '',
    ...overrides,
  }
}

function renderAdsSettingsModal(adsInfo: AdsInfo) {
  const store = defaultAppStore()
  store.update((state) => ({
    adsInfo,
    actions: { ...state.actions, getString: (key) => key },
  }))
  return render(
    <AppContext.Provider value={store}>
      <AdsSettingsModal onClose={() => {}} />
    </AppContext.Provider>,
  )
}

describe('AdsSettingsModal', () => {
  beforeEach(() => {
    // Mock `showModal` since it doesn't compile under jest.
    HTMLDialogElement.prototype.showModal = jest.fn()
  })

  it('show preferences when policies unset', () => {
    renderAdsSettingsModal(
      createAdsInfo({
        adsManagedByPolicy: { 'new-tab-page': false, 'notification': false },
        adsEnabled: { 'new-tab-page': true, 'notification': false },
      }),
    )
    expect(screen.queryByText('adsSettingsAdTypeTitle')).toBeInTheDocument()
    expect(screen.queryByText('adTypeNewTabPageLabel')).toBeInTheDocument()
    expect(screen.queryByText('adTypeNotificationLabel')).toBeInTheDocument()
  })

  it('show new tab page preferences when enabled by policy', () => {
    renderAdsSettingsModal(
      createAdsInfo({
        adsManagedByPolicy: { 'new-tab-page': true, 'notification': false },
        adsEnabled: { 'new-tab-page': true, 'notification': false },
      }),
    )
    expect(screen.queryByText('adsSettingsAdTypeTitle')).toBeInTheDocument()
    expect(screen.queryByText('adTypeNewTabPageLabel')).toBeInTheDocument()
    expect(screen.queryByText('adTypeNotificationLabel')).toBeInTheDocument()
  })

  it('hide new tab page preferences when disabled by policy', () => {
    renderAdsSettingsModal(
      createAdsInfo({
        adsManagedByPolicy: { 'new-tab-page': true, 'notification': false },
        adsEnabled: { 'new-tab-page': false, 'notification': false },
      }),
    )
    expect(screen.queryByText('adsSettingsAdTypeTitle')).toBeInTheDocument()
    expect(screen.queryByText('adTypeNewTabPageLabel')).not.toBeInTheDocument()
    expect(screen.queryByText('adTypeNotificationLabel')).toBeInTheDocument()
  })

  it('show notification preferences when enabled by policy', () => {
    renderAdsSettingsModal(
      createAdsInfo({
        adsManagedByPolicy: { 'new-tab-page': false, 'notification': true },
        adsEnabled: { 'new-tab-page': true, 'notification': true },
      }),
    )
    expect(screen.queryByText('adsSettingsAdTypeTitle')).toBeInTheDocument()
    expect(screen.queryByText('adTypeNewTabPageLabel')).toBeInTheDocument()
    expect(screen.queryByText('adTypeNotificationLabel')).toBeInTheDocument()
  })

  it('hide notification preferences when disabled by policy', () => {
    renderAdsSettingsModal(
      createAdsInfo({
        adsManagedByPolicy: { 'new-tab-page': false, 'notification': true },
        adsEnabled: { 'new-tab-page': true, 'notification': false },
      }),
    )
    expect(screen.queryByText('adsSettingsAdTypeTitle')).toBeInTheDocument()
    expect(screen.queryByText('adTypeNewTabPageLabel')).toBeInTheDocument()
    expect(
      screen.queryByText('adTypeNotificationLabel'),
    ).not.toBeInTheDocument()
  })

  it('show preferences when both enabled by policies', () => {
    renderAdsSettingsModal(
      createAdsInfo({
        adsManagedByPolicy: { 'new-tab-page': true, 'notification': true },
        adsEnabled: { 'new-tab-page': true, 'notification': true },
      }),
    )
    expect(screen.queryByText('adsSettingsAdTypeTitle')).toBeInTheDocument()
    expect(screen.queryByText('adTypeNewTabPageLabel')).toBeInTheDocument()
    expect(screen.queryByText('adTypeNotificationLabel')).toBeInTheDocument()
  })

  it('hide preferences when both disabled by policy', () => {
    renderAdsSettingsModal(
      createAdsInfo({
        adsManagedByPolicy: { 'new-tab-page': true, 'notification': true },
        adsEnabled: { 'new-tab-page': false, 'notification': false },
      }),
    )
    expect(screen.queryByText('adsSettingsAdTypeTitle')).not.toBeInTheDocument()
    expect(screen.queryByText('adTypeNewTabPageLabel')).not.toBeInTheDocument()
    expect(
      screen.queryByText('adTypeNotificationLabel'),
    ).not.toBeInTheDocument()
  })
})
