/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as assert from 'assert'
import * as types from '../../constants/shieldsPanelTypes'
import * as actions from '../../actions/shieldsPanelActions'
import { ShieldDetails, BlockDetails } from '../../types/actions/shieldsPanelActions'
import {
  BlockOptions,
  BlockFPOptions,
  BlockCookiesOptions
} from '../../types/other/blockTypes'

describe('shieldsPanelActions', () => {
  it('shieldsPanelDataUpdated', () => {
    const details: ShieldDetails = {
      ads: 'allow',
      trackers: 'block',
      httpUpgradableResources: 'allow',
      origin: 'https://www.brave.com',
      hostname: 'www.brave.com',
      id: 1,
      javascript: 'allow',
      fingerprinting: 'allow',
      cookies: 'allow'
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
    const setting: BlockOptions = 'allow'
    assert.deepEqual(actions.httpsEverywhereToggled(setting), {
      type: types.HTTPS_EVERYWHERE_TOGGLED,
      setting
    })
  })

  it('javascriptToggled action', () => {
    const setting: BlockOptions = 'allow'
    assert.deepEqual(actions.blockJavaScript(setting), {
      type: types.JAVASCRIPT_TOGGLED,
      setting
    })
  })

  it('resourceBlocked action', () => {
    const details: BlockDetails = {
      blockType: 'ads',
      tabId: 2,
      subresource: 'https://www.brave.com/test'
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

  it('blockFingerprinting action', () => {
    const setting: BlockFPOptions = 'block_third_party'
    assert.deepEqual(actions.blockFingerprinting(setting), {
      type: types.BLOCK_FINGERPRINTING,
      setting
    })
  })

  it('blockCookies action', () => {
    const setting: BlockCookiesOptions = 'block_third_party'
    assert.deepEqual(actions.blockCookies(setting), {
      type: types.BLOCK_COOKIES,
      setting
    })
  })

  it('allowScriptOriginsOnce action', () => {
    const origins = ['https://a.com', 'https://b.com']
    assert.deepEqual(actions.allowScriptOriginsOnce(origins), {
      type: types.ALLOW_SCRIPT_ORIGINS_ONCE,
      origins
    })
  })

  it('changeNoScriptSettings action', () => {
    const origin = 'https://a.com'
    assert.deepEqual(actions.changeNoScriptSettings(origin), {
      type: types.CHANGE_NO_SCRIPT_SETTINGS,
      origin
    })
  })
})
