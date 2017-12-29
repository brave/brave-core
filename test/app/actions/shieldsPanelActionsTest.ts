/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as assert from 'assert'
import * as types from '../../../app/constants/shieldsPanelTypes'
import * as actions from '../../../app/actions/shieldsPanelActions'
import { ShieldDetails, BlockDetails } from '../../../app/types/actions/shieldsPanelActions';
import { BlockOptions } from '../../../app/types/other/blockTypes';

describe('shieldsPanelActions', () => {
  it('shieldsPanelDataUpdated', () => {
    const details: ShieldDetails = {
      ads: 'allow',
      trackers: 'block',
      httpUpgradableResources: 'allow',
      origin: 'https://www.brave.com',
      hostname: 'www.brave.com',
      id: 1,
      javascript: 'allow'
    }
    assert.deepEqual(actions.shieldsPanelDataUpdated(details), {
      type: types.SHIELDS_PANEL_DATA_UPDATED,
      details
    })
  })

  it('shieldsToggled', () => {
    const setting: BlockOptions = 'allow'
    assert.deepEqual(actions.shieldsToggled(setting), {
      type: types.SHIELDS_TOGGLED,
      setting
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
    const details: BlockDetails = {
      blockType: 'ads',
      tabId: 2
    }
    assert.deepEqual(actions.resourceBlocked(details), {
      type: types.RESOURCE_BLOCKED,
      details
    })
  })

  it('blockAdsTrackers action', () => {
    const setting: BlockOptions = 'allow'
    assert.deepEqual(actions.blockAdsTrackers(setting), {
      type: types.BLOCK_ADS_TRACKERS,
      setting
    })
  })

  it('controlsToggled action', () => {
    const setting: boolean = true
    assert.deepEqual(actions.controlsToggled(setting), {
      type: types.CONTROLS_TOGGLED,
      setting
    })
  })
})
