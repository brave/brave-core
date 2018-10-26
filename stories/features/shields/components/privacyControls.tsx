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
} from '../../../../src/features/shields'

// Component groups
import BlockedResources from './blockedReources/blockedResources'
import StaticList from './blockedReources/staticList'
import DynamicList from './blockedReources/dynamicList'

// Fake data
import locale from '../fakeLocale'
import data from '../fakeData'

const scriptsBlockedList = (favicon: string, sitename: string, onToggle: () => void) => {
  return (
    <BlockedResources
      dynamic={true}
      favicon={favicon}
      sitename={sitename}
      title={locale.scriptsOnThisSite}
      onToggle={onToggle}
      data={data.thirdPartyScriptsBlocked}
    >
      <DynamicList onClickDismiss={onToggle} list={data.blockedScriptsResouces} />
    </BlockedResources>
  )
}

const deviceRecognitionList = (favicon: string, sitename: string, onToggle: () => void) => {
  return (
    <BlockedResources
      favicon={favicon}
      sitename={sitename}
      title={locale.deviceRecognitionAttempts}
      onToggle={onToggle}
      data={data.thirdPartyDeviceRecognitionBlocked}
    >
      <StaticList onClickDismiss={onToggle} list={data.blockedFakeResources} />
    </BlockedResources>
  )
}

interface Props {
  enabled: boolean
  sitename: string
  favicon: string
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

  onChangeBlockScripts = (event: React.ChangeEvent<HTMLSelectElement>) => {
    console.log('do something', event.currentTarget)
  }

  onChangeBlockDeviceRecognition = (event: React.ChangeEvent<HTMLSelectElement>) => {
    console.log('do something', event.currentTarget)
  }

  onChangeBlockCookies = (event: React.ChangeEvent<HTMLSelectElement>) => {
    console.log('do something', event.currentTarget)
  }

  render () {
    const { enabled, sitename, favicon } = this.props
    const { openScriptsBlockedList, openDeviceRecognitionList } = this.state

    if (!enabled) {
      return null
    }
    return (
      <>
        {/* cookies select */}
        <SelectGrid hasUserInteraction={false}>
          <EmptyButton />
          <StatFlex />
          <SelectBox onChange={this.onChangeBlockCookies}>
            <option value='block_third_party'>{locale.block3partyCookies}</option>
            <option value='block'>{locale.blockAllCookies}</option>
            <option value='allow'>{locale.allowAllCookies}</option>
          </SelectBox>
          <ClickableEmptySpace />
        </SelectGrid>
        {/* scripts select */}
        <SelectGrid hasUserInteraction={true}>
          <EmptyButton onClick={this.onToggleScriptsBlocked}><ShowMoreIcon /></EmptyButton>
          <StatFlex onClick={this.onToggleScriptsBlocked}>{data.thirdPartyScriptsBlocked}</StatFlex>
          <SelectBox onChange={this.onChangeBlockScripts}>
            <option value='block_third_party'>{locale.blockSomeScripts}</option>
            <option value='block'>{locale.blockAllScripts}</option>
            <option value='allow'>{locale.allowAllScripts}</option>
          </SelectBox>
          <ClickableEmptySpace onClick={this.onToggleScriptsBlocked} />
          {openScriptsBlockedList ? scriptsBlockedList(favicon, sitename, this.onToggleScriptsBlocked) : null}
        </SelectGrid>
        {/* fingerprinting select */}
        <SelectGrid hasUserInteraction={true}>
          <EmptyButton onClick={this.onToggleDeviceRecognition}><ShowMoreIcon /></EmptyButton>
          <StatFlex onClick={this.onToggleDeviceRecognition}>{data.thirdPartyDeviceRecognitionBlocked}</StatFlex>
          <SelectBox onChange={this.onChangeBlockDeviceRecognition}>
            <option value='block_third_party'>{locale.block3partyFingerprinting}</option>
            <option value='block'>{locale.blockAllFingerprinting}</option>
            <option value='allow'>{locale.allowAllFingerprinting}</option>
          </SelectBox>
          <ClickableEmptySpace onClick={this.onToggleDeviceRecognition} />
          {openDeviceRecognitionList ? deviceRecognitionList(favicon, sitename, this.onToggleDeviceRecognition) : null}
        </SelectGrid>
      </>
    )
  }
}
