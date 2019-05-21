/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../../../brave_extension/extension/brave_extension/constants/shieldsPanelTypes'
import * as actions from '../../../brave_extension/extension/brave_extension/actions/shieldsPanelActions'
import { ShieldDetails, BlockDetails } from '../../../brave_extension/extension/brave_extension/types/actions/shieldsPanelActions'
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
    expect(actions.shieldsPanelDataUpdated(details)).toEqual({
      type: types.SHIELDS_PANEL_DATA_UPDATED,
      details
    })
  })

  it('shieldsToggled', () => {
    const setting: BlockOptions = 'allow'
    expect(actions.shieldsToggled(setting)).toEqual({
      type: types.SHIELDS_TOGGLED,
      setting
    })
  })

  it('httpsEverywhereToggled action', () => {
    const setting: BlockOptions = 'allow'
    expect(actions.httpsEverywhereToggled(setting)).toEqual({
      type: types.HTTPS_EVERYWHERE_TOGGLED,
      setting
    })
  })

  it('javascriptToggled action', () => {
    const setting: BlockOptions = 'allow'
    expect(actions.blockJavaScript(setting)).toEqual({
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
    expect(actions.resourceBlocked(details)).toEqual({
      type: types.RESOURCE_BLOCKED,
      details
    })
  })

  it('blockAdsTrackers action', () => {
    const setting: BlockOptions = 'allow'
    expect(actions.blockAdsTrackers(setting)).toEqual({
      type: types.BLOCK_ADS_TRACKERS,
      setting
    })
  })

  it('controlsToggled action', () => {
    const setting: boolean = true
    expect(actions.controlsToggled(setting)).toEqual({
      type: types.CONTROLS_TOGGLED,
      setting
    })
  })

  it('blockFingerprinting action', () => {
    const setting: BlockFPOptions = 'block_third_party'
    expect(actions.blockFingerprinting(setting)).toEqual({
      type: types.BLOCK_FINGERPRINTING,
      setting
    })
  })

  it('blockCookies action', () => {
    const setting: BlockCookiesOptions = 'block_third_party'
    expect(actions.blockCookies(setting)).toEqual({
      type: types.BLOCK_COOKIES,
      setting
    })
  })

  it('allowScriptOriginsOnce action', () => {
    expect(actions.allowScriptOriginsOnce()).toEqual({
      type: types.ALLOW_SCRIPT_ORIGINS_ONCE
    })
  })
})
