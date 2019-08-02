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
  id?: string
}

export default class ModalVerify extends React.PureComponent<Props, {}> {
  getListItem = (text: string) => (
    <StyledListItem>
      <StyledListIcon>
        <RewardsCheckIcon />
      </StyledListIcon>
      <StyledListItemText>
        {text}
      </StyledListItemText>
    </StyledListItem>
  )

  getHeader = (onClose: () => void) => (
    <>
      <StyledClose onClick={onClose}>
        <CloseStrokeIcon />
      </StyledClose>
      <StyledHeader>
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

  getFooter = () => (
    <StyledFooter>
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
      id
    } = this.props

    return (
      <StyledWrapper id={id}>
        {this.getHeader(onClose)}
        <StyledListTitle>
          {getLocale('walletVerificationListHeader')}
        </StyledListTitle>
        {this.getListItem(getLocale('walletVerificationList1'))}
        {this.getListItem(getLocale('walletVerificationList2'))}
        {this.getListItem(getLocale('walletVerificationList3'))}
        <StyledIDNotice>
          {getLocale('walletVerificationID')}
        </StyledIDNotice>
        <StyledButton
          text={getLocale('walletVerificationButton')}
          size={'call-to-action'}
          type={'accent'}
          onClick={onVerifyClick}
          id={'on-boarding-verify-button'}
        />
        {this.getFooter()}
      </StyledWrapper>
    )
  }
}
