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
  compact?: boolean
  id?: string
}

export default class PanelWelcome extends React.PureComponent<Props, {}> {
  getListItem = (text: string, compact?: boolean) => (
    <StyledListItem compact={compact}>
      <StyledListIcon>
        <RewardsCheckIcon />
      </StyledListIcon>
      <StyledListItemText>
        {text}
      </StyledListItemText>
    </StyledListItem>
  )

  getHeader = (onClose: () => void, compact?: boolean) => (
    <>
      <StyledClose onClick={onClose}>
        <CloseStrokeIcon />
      </StyledClose>
      <StyledHeader compact={compact}>
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

  getFooter = (compact?: boolean) => (
    <StyledFooter compact={compact}>
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
      compact,
      id
    } = this.props

    return (
      <StyledWrapper id={id}>
        {this.getHeader(onClose, compact)}
        <StyledListTitle>
          {getLocale('walletVerificationListHeader')}
        </StyledListTitle>
        {this.getListItem(getLocale(compact ? 'walletVerificationListCompact1' : 'walletVerificationList1'), compact)}
        {this.getListItem(getLocale(compact ? 'walletVerificationListCompact2' : 'walletVerificationList2'), compact)}
        {this.getListItem(getLocale(compact ? 'walletVerificationListCompact3' : 'walletVerificationList3'), compact)}
        <StyledIDNotice compact={compact}>
          {getLocale('walletVerificationID')}
        </StyledIDNotice>
        <StyledButton
          text={getLocale('walletVerificationButton')}
          size={'call-to-action'}
          type={'accent'}
          onClick={onVerifyClick}
          id={'on-boarding-verify-button'}
        />
        {this.getFooter(compact)}
      </StyledWrapper>
    )
  }
}
