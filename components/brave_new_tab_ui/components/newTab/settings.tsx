/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { SettingsMenu, SettingsRow, SettingsText, SettingsTitle, SettingsWrapper } from 'brave-ui/features/newTab/default'

import { Toggle } from 'brave-ui/features/newTab/toggle'

import { getLocale } from '../../../common/locale'

export interface Props {
  onClickOutside: () => void
  toggleShowBackgroundImage: () => void
  showBackgroundImage: boolean
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

  render () {
    const { toggleShowBackgroundImage, showBackgroundImage } = this.props
    return (
      <SettingsWrapper>
        <SettingsMenu innerRef={this.settingsMenuRef}>
          <SettingsTitle>{getLocale('dashboardSettingsTitle')}</SettingsTitle>
          <SettingsRow>
            <SettingsText>{getLocale('showBackgroundImage')}</SettingsText>
            <Toggle
              onChange={toggleShowBackgroundImage}
              checked={showBackgroundImage}
              size='small'
            />
          </SettingsRow>
        </SettingsMenu>
      </SettingsWrapper>
    )
  }
}
