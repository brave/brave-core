/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  SelectBox,
  Stat,
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

  get scriptsBlockedList () {
    const { favicon, sitename } = this.props
    return (
      <BlockedResources
        dynamic={true}
        favicon={favicon}
        sitename={sitename}
        title={locale.scriptsOnThisSite}
        onToggle={this.onToggleScriptsBlocked}
        data={data.thirdPartyScriptsBlocked}
      >
        <DynamicList
          onClickDismiss={this.onToggleScriptsBlocked}
          list={data.blockedScriptsResouces}
        />
      </BlockedResources>
    )
  }

  get deviceRecognitionList () {
    const { favicon, sitename } = this.props
    return (
      <BlockedResources
        favicon={favicon}
        sitename={sitename}
        title={locale.deviceRecognitionAttempts}
        onToggle={this.onToggleDeviceRecognition}
        data={data.thirdPartyDeviceRecognitionBlocked}
      >
        <StaticList
          onClickDismiss={this.onToggleDeviceRecognition}
          list={data.blockedFakeResources}
        />
      </BlockedResources>
    )
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
    const { enabled } = this.props
    const { openScriptsBlockedList, openDeviceRecognitionList } = this.state

    if (!enabled) {
      return null
    }
    return (
      <>
        {/* cookies select */}
        <SelectGrid hasUserInteraction={false}>
          <EmptyButton />
          <Stat /> {/* {data.thirdPartyCookiesBlocked}</Stat> */}
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
          <Stat onClick={this.onToggleScriptsBlocked}>{data.thirdPartyScriptsBlocked}</Stat>
          <SelectBox onChange={this.onChangeBlockScripts}>
            <option value='block_third_party'>{locale.blockSomeScripts}</option>
            <option value='block'>{locale.blockAllScripts}</option>
            <option value='allow'>{locale.allowAllScripts}</option>
          </SelectBox>
          <ClickableEmptySpace onClick={this.onToggleScriptsBlocked} />
          {openScriptsBlockedList ? this.scriptsBlockedList : null}
        </SelectGrid>
        {/* fingerprinting select */}
        <SelectGrid hasUserInteraction={true}>
          <EmptyButton onClick={this.onToggleDeviceRecognition}><ShowMoreIcon /></EmptyButton>
          <Stat onClick={this.onToggleDeviceRecognition}>{data.thirdPartyDeviceRecognitionBlocked}</Stat>
          <SelectBox onChange={this.onChangeBlockDeviceRecognition}>
            <option value='block_third_party'>{locale.block3partyFingerprinting}</option>
            <option value='block'>{locale.blockAllFingerprinting}</option>
            <option value='allow'>{locale.allowAllFingerprinting}</option>
          </SelectBox>
          <ClickableEmptySpace onClick={this.onToggleDeviceRecognition} />
          {openDeviceRecognitionList ? this.deviceRecognitionList : null}
        </SelectGrid>
      </>
    )
  }
}
