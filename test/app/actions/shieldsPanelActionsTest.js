/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import assert from 'assert'
import * as types from '../../../app/constants/shieldsPanelTypes'
import * as actions from '../../../app/actions/shieldsPanelActions'

describe('shieldsPanelActions', () => {
  it('shieldsPanelDataUpdated', () => {
    const details = {
      adBlock: 'allow',
      trackingProtection: 'block',
      httpsEverywhere: 'allow',
      origin: 'https://www.brave.com',
      hostname: 'www.brave.com'
    }
    assert.deepEqual(actions.shieldsPanelDataUpdated(details), {
      type: types.SHIELDS_PANEL_DATA_UPDATED,
      details
    })
  })

  it('shieldsToggled', () => {
    const setting = 'allow'
    assert.deepEqual(actions.shieldsToggled(), {
      type: types.SHIELDS_TOGGLED,
      setting
    })
  })

  it('adBlockToggled action', () => {
    assert.deepEqual(actions.adBlockToggled(), {
      type: types.AD_BLOCK_TOGGLED
    })
  })

  it('trackingProtectionToggled action', () => {
    assert.deepEqual(actions.trackingProtectionToggled(), {
      type: types.TRACKING_PROTECTION_TOGGLED
    })
  })

  it('httpsEverywhereToggled action', () => {
    assert.deepEqual(actions.httpsEverywhereToggled(), {
      type: types.HTTPS_EVERYWHERE_TOGGLED
    })
  })

  it('javascriptToggled action', () => {
    assert.deepEqual(actions.javascriptToggled(), {
      type: types.JAVASCRIPT_TOGGLED
    })
  })

  it('resourceBlocked action', () => {
    const details = {
      blockType: 'adBlock',
      tabId: 2
    }
    assert.deepEqual(actions.resourceBlocked(details), {
      type: types.RESOURCE_BLOCKED,
      details
    })
  })

  it('blockAdsTrackers action', () => {
    const setting = 'allow'
    assert.deepEqual(actions.blockAdsTrackers(setting), {
      type: types.BLOCK_ADS_TRACKERS,
      setting
    })
  })

  it('controlsToggled action', () => {
    const setting = true
    assert.deepEqual(actions.controlsToggled(setting), {
      type: types.CONTROLS_TOGGLED,
      setting
    })
  })
})
