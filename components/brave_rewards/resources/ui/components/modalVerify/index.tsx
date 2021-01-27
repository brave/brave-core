/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from 'brave-ui/helpers'

import {
  StyledWrapper,
  StyledClose,
  StyledHeader,
  StyledWalletIcon,
  StyledHeaderText,
  StyledTitle,
  StyledListTitle,
  StyledListItem,
  StyledListIcon,
  StyledListItemText,
  StyledButton,
  StyledFooter,
  StyledFooterIcon,
  StyledLeftSide,
  StyledRightSide,
  StyledContent,
  StyledNote,
  NoteText
} from './style'

import {
  CloseStrokeIcon,
  UpholdColorIcon,
  RewardsWalletCheck,
  RewardsCheckIcon
} from 'brave-ui/components/icons'
import { Modal } from 'brave-ui/components'

import { BitflyerIcon } from '../../../shared/components/icons/bitflyer_icon'

export interface Props {
  onVerifyClick: () => void
  onClose: () => void
  id?: string
  walletType?: string
  walletProvider: string
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
        <StyledWalletIcon>
          <RewardsWalletCheck />
        </StyledWalletIcon>
        <StyledHeaderText>
          <StyledTitle level={1}>
            {getLocale('walletVerificationTitle1')}
          </StyledTitle>
        </StyledHeaderText>
      </StyledHeader>
    </>
  )

  getFooter = () => {
    const { walletType, walletProvider } = this.props
    const tags = getLocale('walletVerificationFooter').split(/\$\d/g)
    return (
      <StyledFooter>
        <span>
          {tags[0]}
          <b>{walletProvider}</b>
          {tags[1]}
        </span>
        <StyledFooterIcon>
          {
            walletType === 'uphold' ? <UpholdColorIcon /> :
            walletType === 'bitflyer' ? <BitflyerIcon white={true} /> :
            null
          }
        </StyledFooterIcon>
      </StyledFooter>
    )
  }

  render () {
    const {
      onVerifyClick,
      onClose,
      id,
      walletProvider
    } = this.props

    return (
      <Modal id={id} displayCloseButton={false} size={'small'}>
        <StyledWrapper id={id}>
          {this.getHeader(onClose)}
          <StyledListTitle>
            {getLocale('walletVerificationListHeader')}
          </StyledListTitle>
          <StyledContent>
            <StyledLeftSide>
              {this.getListItem(getLocale('walletVerificationList1'))}
              {this.getListItem(getLocale('walletVerificationList2'))}
              {this.getListItem(getLocale('walletVerificationList3'))}
            </StyledLeftSide>
            <StyledRightSide>
              <StyledButton
                text={getLocale('walletVerificationButton')}
                size={'call-to-action'}
                type={'accent'}
                onClick={onVerifyClick}
                id={'on-boarding-verify-button'}
              />
              {this.getFooter()}
              <StyledNote>
                {getLocale('walletVerificationNote1').replace(/\$1/g, walletProvider)}
                <br/><br/>
                {getLocale('walletVerificationNote2').replace(/\$1/g, walletProvider)}
              </StyledNote>
            </StyledRightSide>
          </StyledContent>
          <NoteText>
            {getLocale('walletVerificationNote3').replace(/\$1/g, walletProvider)}
          </NoteText>
        </StyledWrapper>
      </Modal>
    )
  }
}
