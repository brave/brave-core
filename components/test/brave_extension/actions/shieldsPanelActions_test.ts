/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Types
import * as types from '../../../brave_extension/extension/brave_extension/constants/shieldsPanelTypes'
import { ShieldDetails, BlockDetails } from '../../../brave_extension/extension/brave_extension/types/actions/shieldsPanelActions'
import {
  BlockOptions,
  BlockFPOptions,
  BlockCookiesOptions
} from '../../../brave_extension/extension/brave_extension/types/other/blockTypes'

// Actions
import * as actions from '../../../brave_extension/extension/brave_extension/actions/shieldsPanelActions'

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

describe('shieldsPanelActions', () => {
  it('shieldsPanelDataUpdated', () => {
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
      blockType: 'shieldsAds',
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

  it('setScriptBlockedCurrentState', () => {
    const url = 'https://awesome-anthony-tseng.website'
    expect(actions.setScriptBlockedCurrentState(url)).toEqual({
      type: types.SET_SCRIPT_BLOCKED_ONCE_CURRENT_STATE,
      url
    })
  })

  it('setGroupedScriptsBlockedCurrentState', () => {
    const origin = 'https://clifton.io'
    const maybeBlock = false
    expect(actions.setGroupedScriptsBlockedCurrentState(origin, maybeBlock)).toEqual({
      type: types.SET_GROUPED_SCRIPTS_BLOCKED_ONCE_CURRENT_STATE,
      origin,
      maybeBlock
    })
  })

  it('setAllScriptsBlockedCurrentState', () => {
    const maybeBlock = true
    expect(actions.setAllScriptsBlockedCurrentState(maybeBlock)).toEqual({
      type: types.SET_ALL_SCRIPTS_BLOCKED_ONCE_CURRENT_STATE,
      maybeBlock
    })
  })

  it('setFinalScriptsBlockedState', () => {
    expect(actions.setFinalScriptsBlockedState()).toEqual({
      type: types.SET_FINAL_SCRIPTS_BLOCKED_ONCE_STATE
    })
  })

  it('setAdvancedViewFirstAccess', () => {
    expect(actions.setAdvancedViewFirstAccess()).toEqual({
      type: types.SET_ADVANCED_VIEW_FIRST_ACCESS
    })
  })
})
