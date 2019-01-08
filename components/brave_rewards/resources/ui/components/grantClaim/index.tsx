/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledIcon, StyledText, StyledClaim } from './style'
import { getLocale } from '../../../helpers'
import { GiftIcon, MegaphoneIcon } from '../../../components/icons'

export type Type = 'ads' | 'ugp'

export interface Props {
  id?: string
  isMobile?: boolean
  onClaim: () => void
  type: Type
  amount?: string
}

export default class GrantClaim extends React.PureComponent<Props, {}> {
  getIcon = (type: Type) => {
    let icon = null

    switch (type) {
      case 'ads':
        icon = <MegaphoneIcon />
        break
      case 'ugp':
        icon = <GiftIcon />
        break
    }

    return icon
  }

  getGrantText = (type: Type, amount?: string) => {
    if (type === 'ugp') {
      return getLocale('newGrant')
    }

    if (!amount) {
      return getLocale('earningsClaimDefault')
    }

    const formattedAmount = `${amount} ${getLocale('earningsClaimBat')}`

    return (
      <>
        {getLocale('earningsClaimOne')} <b>{formattedAmount}</b> {getLocale('earningsClaimTwo')}
      </>
    )
  }

  render () {
    const { id, isMobile, onClaim, type, amount } = this.props

    return (
      <StyledWrapper
        id={id}
        isMobile={isMobile}
      >
        <StyledIcon type={type}>
          {this.getIcon(type)}
        </StyledIcon>
        <StyledText>
          {this.getGrantText(type, amount)}
        </StyledText>
        <StyledClaim onClick={onClaim}>{getLocale('claim')}</StyledClaim>
      </StyledWrapper>
    )
  }
}
