/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledContent,
  StyledTwoColumn,
  StyledIcon
} from './style'
import {
  AdsTokensIcon,
  AutoContributeIcon,
  MonthlyContributionsIcon,
  TipsIcon
} from 'brave-ui/components/icons'

export type Type = 'ads' | 'contribute' | 'donation' | 'monthly'

export interface Props {
  children: React.ReactNode
  id?: string
  type?: Type
}

export default class DisabledContent extends React.PureComponent<Props, {}> {
  getIcon = (type?: Type) => {
    let icon = null

    switch (type) {
      case 'ads':
        icon = <AdsTokensIcon />
        break
      case 'contribute':
        icon = <AutoContributeIcon />
        break
      case 'donation':
        icon = <TipsIcon />
        break
      case 'monthly':
        icon = <MonthlyContributionsIcon />
        break
    }

    return (
      <StyledIcon>
        {icon}
      </StyledIcon>
    )
  }

  render () {
    const { id, children, type } = this.props

    return (
      <div id={id}>
        <StyledTwoColumn>
          {this.getIcon(type)}
          <StyledContent type={type}>
            {children}
          </StyledContent>
        </StyledTwoColumn>
      </div>
    )
  }
}
