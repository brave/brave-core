/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { SettingsMenu, SettingsRow, SettingsText, SettingsTitle, SettingsWrapper, IconButton } from '../../components/default'

import { Toggle } from '../../components/toggle'

import { getLocale } from '../fakeLocale'
import { SettingsIcon } from 'brave-ui/components/icons'

interface Props {
  textDirection: string
  showSettingsMenu: boolean
  onClick: () => void
  onClickOutside: () => void
  toggleShowBackgroundImage: () => void
  toggleShowClock: () => void
  toggleShowStats: () => void
  toggleShowTopSites: () => void
  toggleShowRewards: () => void
  showBackgroundImage: boolean
  showStats: boolean
  showClock: boolean
  showTopSites: boolean
  showRewards: boolean
}

export default class Settings extends React.PureComponent<Props, {}> {
  settingsMenuRef: React.RefObject<any>
  constructor (props: Props) {
    super(props)
    this.settingsMenuRef = React.createRef()
  }

  handleClickOutside = (event: Event) => {
    if (this.settingsMenuRef && !this.settingsMenuRef.current.contains(event.target)) {
      this.props.onClickOutside()
    }
  }

  componentDidMount () {
    document.addEventListener('mousedown', this.handleClickOutside)
  }

  componentWillUnmount () {
    document.removeEventListener('mousedown', this.handleClickOutside)
  }

  onKeyPressSettings = (event: React.KeyboardEvent<HTMLButtonElement>) => {
    if (event.key === 'Escape') {
      this.props.onClickOutside()
    }
  }

  render () {
    const {
      textDirection,
      showSettingsMenu,
      toggleShowBackgroundImage,
      toggleShowClock,
      toggleShowStats,
      toggleShowTopSites,
      toggleShowRewards,
      showBackgroundImage,
      showStats,
      showClock,
      showTopSites,
      showRewards,
      onClick
    } = this.props
    return (
      <SettingsWrapper innerRef={this.settingsMenuRef}>
        <IconButton onClick={onClick} onKeyDown={this.onKeyPressSettings}><SettingsIcon/></IconButton>
        {showSettingsMenu &&
          <SettingsMenu textDirection={textDirection}>
            <SettingsTitle>{getLocale('dashboardSettingsTitle')}</SettingsTitle>
            <SettingsRow>
              <SettingsText>{getLocale('showBackgroundImg')}</SettingsText>
              <Toggle
                onChange={toggleShowBackgroundImage}
                checked={showBackgroundImage}
                size='small'
              />
            </SettingsRow>
            <SettingsRow>
              <SettingsText>{getLocale('showBraveStats')}</SettingsText>
              <Toggle
                onChange={toggleShowStats}
                checked={showStats}
                size='small'
              />
            </SettingsRow>
            <SettingsRow>
              <SettingsText>{getLocale('showClock')}</SettingsText>
              <Toggle
                onChange={toggleShowClock}
                checked={showClock}
                size='small'
              />
            </SettingsRow>
            <SettingsRow>
              <SettingsText>{getLocale('showTopSites')}</SettingsText>
              <Toggle
                onChange={toggleShowTopSites}
                checked={showTopSites}
                size='small'
              />
            </SettingsRow>
            <SettingsRow>
              <SettingsText>{getLocale('showRewards')}</SettingsText>
              <Toggle
                onChange={toggleShowRewards}
                checked={showRewards}
                size='small'
              />
            </SettingsRow>
        </SettingsMenu>
      }
    </SettingsWrapper>
    )
  }
}
