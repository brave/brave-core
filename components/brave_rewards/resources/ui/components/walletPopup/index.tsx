/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Helpers
import { getLocale } from 'brave-ui/helpers'
import { WalletState } from '../walletWrapper/index'

// Styled Components
import {
  StyledContent,
  StyledDialog,
  StyledHeader,
  StyledIcon,
  StyledStatus,
  StyledWrapper
} from './style'
import {
  UpholdColorIcon
} from 'brave-ui/components/icons'

import { BitflyerIcon } from '../../../shared/components/icons/bitflyer_icon'

export interface Props {
  children: React.ReactNode
  onClose: () => void
  greetings: string
  id?: string
  walletState?: WalletState
  walletType?: string
}

export default class WalletPopup extends React.PureComponent<Props, {}> {
  insideClick = (e: React.SyntheticEvent) => {
    // Don't propogate click to container, which will close it
    e.stopPropagation()
  }

  isVerified () {
    const { walletState } = this.props
    return walletState === 'verified'
  }

  getStatusMessage () {
    const { walletState } = this.props
    switch (walletState) {
      case 'verified': {
        return getLocale('walletVerified')
      }
      case 'connected': {
        return getLocale('walletConnected')
      }
      case 'pending': {
        return getLocale('walletPending')
      }
    }

    return ''
  }

  render () {
    const {
      children,
      onClose,
      greetings,
      walletType,
      id
    } = this.props
    return (
      <StyledWrapper onClick={onClose} id={id}>
        <StyledDialog onClick={this.insideClick}>
          <StyledContent>
            <StyledHeader>
              <StyledIcon>
                {
                  walletType === 'uphold' ? <UpholdColorIcon /> :
                  walletType === 'bitflyer' ? <BitflyerIcon /> :
                  null
                }
              </StyledIcon>
              {greetings}
              <StyledStatus isVerified={this.isVerified()}>
                {this.getStatusMessage()}
              </StyledStatus>
            </StyledHeader>
            {children}
          </StyledContent>
        </StyledDialog>
      </StyledWrapper>
    )
  }
}
