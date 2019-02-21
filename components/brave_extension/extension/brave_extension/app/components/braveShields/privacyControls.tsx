/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  SelectBox,
  StatFlex,
  ClickableEmptySpace,
  EmptyButton,
  ShowMoreIcon,
  SelectGrid
} from 'brave-ui/features/shields'

// Component groups
import BlockedResources from './blockedResources/blockedResources'
import StaticList from './blockedResources/staticList'
import ScriptsList from './blockedResources/scriptsList'

// Types
import { BlockOptions, BlockCookiesOptions, BlockJSOptions, BlockFPOptions } from '../../types/other/blockTypes'
import { NoScriptInfo } from '../../types/other/noScriptInfo'

// Utils
import * as shieldActions from '../../types/actions/shieldsPanelActions'
import { getMessage } from '../../background/api/localeAPI'
import { blockedResourcesSize } from '../../helpers/shieldsUtils'

export interface Props {
  url: string
  hostname: string
  origin: string
  braveShields: BlockOptions
  fingerprinting: BlockFPOptions
  fingerprintingBlocked: number
  blockFingerprinting: shieldActions.BlockFingerprinting
  fingerprintingBlockedResources: Array<string>
  javascript: BlockJSOptions
  javascriptBlocked: number
  blockJavaScript: shieldActions.BlockJavaScript
  javascriptBlockedResources: Array<string>
  noScriptInfo: NoScriptInfo
  changeAllNoScriptSettings: shieldActions.ChangeAllNoScriptSettings
  changeNoScriptSettings: shieldActions.ChangeNoScriptSettings
  allowScriptOriginsOnce: shieldActions.AllowScriptOriginsOnce
  blockCookies: shieldActions.BlockCookies
  cookies: BlockCookiesOptions
}

interface State {
  openScriptsBlockedList: boolean
  openDeviceRecognitionList: boolean
}

export default class PrivacyControls extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      openScriptsBlockedList: false,
      openDeviceRecognitionList: false
    }
  }

  onToggleScriptsBlocked = () => {
    this.setState({ openScriptsBlockedList: !this.state.openScriptsBlockedList })
  }

  onToggleDeviceRecognition = () => {
    this.setState({ openDeviceRecognitionList: !this.state.openDeviceRecognitionList })
  }

  onChangeBlockCookies = (event: React.ChangeEvent<any>) => {
    if (!event.target) {
      return
    }
    this.props.blockCookies(event.target.value)
  }

  onChangeBlockScripts = (event: React.ChangeEvent<any>) => {
    if (!event.target) {
      return
    }
    this.props.blockJavaScript(event.target.value)
  }

  onChangeBlockDeviceRecognition = (event: React.ChangeEvent<any>) => {
    if (!event.target) {
      return
    }
    this.props.blockFingerprinting(event.target.value)
  }

  renderScriptsBlockedList = () => {
    const {
      url,
      hostname,
      origin,
      noScriptInfo,
      javascript,
      blockJavaScript,
      changeNoScriptSettings,
      allowScriptOriginsOnce,
      changeAllNoScriptSettings
    } = this.props
    return (
      <BlockedResources
        dynamic={true}
        url={url}
        hostname={hostname}
        title={getMessage('scriptsOnThisSite')}
        onToggle={this.onToggleScriptsBlocked}
        stats={undefined}
      >
        <ScriptsList
          origin={origin}
          onClickDismiss={this.onToggleScriptsBlocked}
          changeNoScriptSettings={changeNoScriptSettings}
          allowScriptOriginsOnce={allowScriptOriginsOnce}
          noScriptInfo={noScriptInfo}
          changeAllNoScriptSettings={changeAllNoScriptSettings}
          javascript={javascript}
          blockJavaScript={blockJavaScript}
        />
      </BlockedResources>
    )
  }

  renderDeviceRecognitionList = () => {
    const { url, hostname, fingerprintingBlocked, fingerprintingBlockedResources } = this.props
    return (
      <BlockedResources
        url={url}
        hostname={hostname}
        title={getMessage('deviceRecognitionAttempts')}
        onToggle={this.onToggleDeviceRecognition}
        stats={fingerprintingBlocked}
      >
        <StaticList onClickDismiss={this.onToggleDeviceRecognition} list={fingerprintingBlockedResources} />
      </BlockedResources>
    )
  }

  render () {
    const {
      braveShields,
      fingerprinting,
      fingerprintingBlocked,
      javascript,
      javascriptBlocked,
      cookies
    } = this.props
    const { openScriptsBlockedList, openDeviceRecognitionList } = this.state
    const enabled = braveShields !== 'block'

    if (!enabled) {
      return null
    }

    return (
      <div id='braveShieldsPrivacyControls'>
        {/* cookies select */}
        <SelectGrid hasUserInteraction={false}>
          <EmptyButton disabled={true} />
          <StatFlex disabled={true} />
          <SelectBox id='blockCookies' value={cookies} onChange={this.onChangeBlockCookies}>
            <option value='block_third_party'>{getMessage('block3partyCookies')}</option>
            <option value='block'>{getMessage('blockAllCookies')}</option>
            <option value='allow'>{getMessage('allowAllCookies')}</option>
          </SelectBox>
          <ClickableEmptySpace />
        </SelectGrid>

        {/* scripts select */}
        <SelectGrid hasUserInteraction={javascriptBlocked !== 0}>
          <EmptyButton disabled={javascriptBlocked === 0} onClick={this.onToggleScriptsBlocked}><ShowMoreIcon /></EmptyButton>
          <StatFlex disabled={javascriptBlocked === 0} id='blockScriptsStat' onClick={this.onToggleScriptsBlocked}>{blockedResourcesSize(javascriptBlocked)}</StatFlex>
          <SelectBox id='blockScripts' value={javascript} onChange={this.onChangeBlockScripts}>
            <option value='block'>{getMessage('blockAllScriptsOrigins')}</option>
            <option value='allow'>{getMessage('allowAllScripts')}</option>
          </SelectBox>
          <ClickableEmptySpace disabled={javascriptBlocked === 0} onClick={this.onToggleScriptsBlocked} />
          {
            openScriptsBlockedList
              ? this.renderScriptsBlockedList()
              : null
          }
        </SelectGrid>

        {/* fingerprinting select */}
        <SelectGrid hasUserInteraction={fingerprintingBlocked !== 0}>
          <EmptyButton disabled={fingerprintingBlocked === 0} onClick={this.onToggleDeviceRecognition}><ShowMoreIcon /></EmptyButton>
          <StatFlex disabled={fingerprintingBlocked === 0} id='blockFingerprintingStat' onClick={this.onToggleDeviceRecognition}>{blockedResourcesSize(fingerprintingBlocked)}</StatFlex>
          <SelectBox id='blockFingerprinting' value={fingerprinting} onChange={this.onChangeBlockDeviceRecognition}>
            <option value='block_third_party'>{getMessage('block3partyFingerprinting')}</option>
            <option value='block'>{getMessage('blockAllFingerprinting')}</option>
            <option value='allow'>{getMessage('allowAllFingerprinting')}</option>
          </SelectBox>
          <ClickableEmptySpace disabled={fingerprintingBlocked === 0} onClick={this.onToggleDeviceRecognition} />
          {
            openDeviceRecognitionList
              ? this.renderDeviceRecognitionList()
              : null
          }
        </SelectGrid>
      </div>
    )
  }
}
