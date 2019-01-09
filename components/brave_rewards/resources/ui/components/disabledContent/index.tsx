/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledContent,
  StyledGrid,
  StyledOneColumn,
  StyledTwoColumn,
  StyledIcon
} from './style'
import {
  AdsMegaphoneIcon,
  RewardsActivateIcon,
  RewardsSendTipsIcon
} from '../../../components/icons'

export type Type = 'ads' | 'contribute' | 'donation'

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
        icon = <AdsMegaphoneIcon />
        break
      case 'contribute':
        icon = <RewardsActivateIcon />
        break
      case 'donation':
        icon = <RewardsSendTipsIcon />
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
        <StyledGrid>
          <StyledOneColumn>
            {this.getIcon(type)}
          </StyledOneColumn>
          <StyledTwoColumn>
            <StyledContent type={type}>
              {children}
            </StyledContent>
          </StyledTwoColumn>
        </StyledGrid>
      </div>
    )
  }
}
