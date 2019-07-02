/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '../../../helpers'

import {
  StyledWrapper,
  StyledClose,
  StyledHeader,
  StyledBatIcon,
  StyledHeaderText,
  StyledTitle,
  StyledSubtitle,
  StyledListTitle,
  StyledListItem,
  StyledListIcon,
  StyledListItemText,
  StyledIDNotice,
  StyledButton,
  StyledFooter,
  StyledFooterIcon
} from './style'

import {
  CloseStrokeIcon,
  UpholdColorIcon,
  RewardsWalletCheck,
  RewardsCheckIcon
} from '../../../components/icons'

export interface Props {
  onVerifyClick: () => void
  onClose: () => void
  short?: boolean
  id?: string
}

export default class PanelWelcome extends React.PureComponent<Props, {}> {
  getListItem = (text: string, short?: boolean) => (
    <StyledListItem short={short}>
      <StyledListIcon>
        <RewardsCheckIcon />
      </StyledListIcon>
      <StyledListItemText>
        {text}
      </StyledListItemText>
    </StyledListItem>
  )

  getHeader = (onClose: () => void, short?: boolean) => (
    <>
      <StyledClose onClick={onClose}>
        <CloseStrokeIcon />
      </StyledClose>
      <StyledHeader short={short}>
        <StyledBatIcon>
          <RewardsWalletCheck />
        </StyledBatIcon>
        <StyledHeaderText>
          <StyledTitle level={1}>
            {getLocale('walletVerificationTitle1')}
          </StyledTitle>
          <StyledSubtitle level={2}>
            {getLocale('walletVerificationTitle2')}
          </StyledSubtitle>
        </StyledHeaderText>
      </StyledHeader>
    </>
  )

  getFooter = (short?: boolean) => (
    <StyledFooter short={short}>
      {getLocale('walletVerificationFooter')} <b>Uphold</b>
      <StyledFooterIcon>
        <UpholdColorIcon />
      </StyledFooterIcon>
    </StyledFooter>
  )

  render () {
    const {
      onVerifyClick,
      onClose,
      short,
      id
    } = this.props

    return (
      <StyledWrapper id={id}>
        {this.getHeader(onClose, short)}
        <StyledListTitle>
          {getLocale('walletVerificationListHeader')}
        </StyledListTitle>
        {this.getListItem(getLocale(short ? 'walletVerificationListShort1' : 'walletVerificationList1'), short)}
        {this.getListItem(getLocale(short ? 'walletVerificationListShort2' : 'walletVerificationList2'), short)}
        {this.getListItem(getLocale(short ? 'walletVerificationListShort3' : 'walletVerificationList3'), short)}
        <StyledIDNotice short={short}>
          {getLocale('walletVerificationID')}
        </StyledIDNotice>
        <StyledButton
          text={getLocale('walletVerificationButton')}
          size={'call-to-action'}
          type={'accent'}
          onClick={onVerifyClick}
        />
        {this.getFooter(short)}
      </StyledWrapper>
    )
  }
}
