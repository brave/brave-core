// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { BraveWallet } from '../../../constants/types'

// Styled Components
import {
  StyledWrapper,
  AppDescColumn,
  AppDesctription,
  AppIconWrapper,
  AppIcon,
  AppName,
  IconAndInfo,
  SelectedIcon,
  UnSelectedIcon
} from './style'

export interface Props {
  appInfo: BraveWallet.AppItem
  isStared: boolean
  toggleFavorite: () => void
}

export default class AppListItem extends React.PureComponent<Props> {
  getSrc (src?: string) {
    return src || ''
  }

  openApp = () => {
    window.open(this.props.appInfo.url, '_blank', 'noreferrer')
  }

  render () {
    const { appInfo, isStared, toggleFavorite } = this.props
    return (
      <StyledWrapper>
        <IconAndInfo>
          <AppIconWrapper>
            <AppIcon src={this.getSrc(appInfo.icon)} />
          </AppIconWrapper>
          <AppDescColumn>
            <AppName onClick={this.openApp}>{appInfo.name}</AppName>
            <AppDesctription>{appInfo.description}</AppDesctription>
          </AppDescColumn>
        </IconAndInfo>
        {isStared ? <SelectedIcon onClick={toggleFavorite} /> : <UnSelectedIcon onClick={toggleFavorite} />}
      </StyledWrapper>
    )
  }
}
