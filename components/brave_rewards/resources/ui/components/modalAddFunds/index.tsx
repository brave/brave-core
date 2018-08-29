/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledNote,
  StyledAddresses,
  StyledAddress,
  StyledLogo,
  StyledData,
  StyledQRTitle,
  StyledQRImageWrapper,
  StyledQRImage,
  StyledCard,
  StyledAddressTitle
} from './style'
import Modal from '../../../components/popupModals/modal/index'
import { getLocale } from '../../../helpers'
import { BatColorIcon, BitcoinColorIcon, EthereumColorIcon, LitecoinColorIcon } from '../../../components/icons'

export type Type = 'BAT' | 'ETH' | 'BTC' | 'LTC'

export interface Address {
  address: string
  qr: string
  type: Type
}

export interface Props {
  onClose: () => void
  id?: string
  addresses: Address[]
}

const icons: Record<Type, React.ReactNode> = {
  BAT: <BatColorIcon />,
  ETH: <EthereumColorIcon />,
  BTC: <BitcoinColorIcon />,
  LTC: <LitecoinColorIcon />
}

export default class ModalAddFunds extends React.PureComponent<Props, {}> {
  getAddress = (address: Address) => {

    return (
      <StyledAddress>
        <StyledCard>
          <StyledLogo>
            {icons[address.type]}
          </StyledLogo>
          <StyledAddressTitle>
            {getLocale(`title${address.type}`)}
          </StyledAddressTitle>
          <StyledData>
            {address.address}
          </StyledData>
          <StyledQRTitle>{getLocale('addFundsQR')}</StyledQRTitle>
          <StyledQRImageWrapper>
            <StyledQRImage src={address.qr} />
          </StyledQRImageWrapper>
        </StyledCard>
      </StyledAddress>
    )
  }

  render () {
    const { id, onClose, addresses } = this.props

    return (
      <Modal id={id} onClose={onClose}>
        <StyledWrapper>
          <StyledTitle>{getLocale('addFundsTitle')}</StyledTitle>
          <StyledAddresses>
            {
              addresses && addresses.map((address: Address) => this.getAddress(address))
            }
          </StyledAddresses>
          <StyledNote>
            {getLocale('addFundsNote')} <a href='https://brave.com/faq-payments/#brave-payments'>{getLocale('addFundsFAQ')}</a>.
          </StyledNote>
        </StyledWrapper>
      </Modal>
    )
  }
}
