/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Helpers
import { getLocale } from 'brave-ui/helpers'

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

export interface Props {
  children: React.ReactNode
  onClose: () => void
  userName: string
  id?: string
  verified?: boolean
}

export default class WalletPopup extends React.PureComponent<Props, {}> {
  insideClick = (e: React.SyntheticEvent) => {
    // Don't propogate click to container, which will close it
    e.stopPropagation()
  }

  render () {
    const {
      children,
      onClose,
      userName,
      id,
      verified
    } = this.props
    return (
      <StyledWrapper onClick={onClose} id={id}>
        <StyledDialog onClick={this.insideClick}>
          <StyledContent>
            <StyledHeader>
              <StyledIcon>
                <UpholdColorIcon />
              </StyledIcon>
              {userName}
              {
                verified
                ? <StyledStatus>
                  {getLocale('walletVerified')}
                </StyledStatus>
                : null
              }
            </StyledHeader>
            {children}
          </StyledContent>
        </StyledDialog>
      </StyledWrapper>
    )
  }
}
