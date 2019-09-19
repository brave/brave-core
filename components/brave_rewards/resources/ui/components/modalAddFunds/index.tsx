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
  StyledShowQR,
  StyledQRImageWrapper,
  StyledQRImage,
  StyledAddressTitle,
  StyledLink,
  StyledHeader,
  StyledWalletAddress,
  StyledQRButton,
  StyledText
} from './style'
import Modal from 'brave-ui/components/popupModals/modal/index'
import { getLocale } from 'brave-ui/helpers'
import Input from 'brave-ui/components/formControls/input'
import { BatColorIcon, BitcoinColorIcon, EthereumColorIcon, LitecoinColorIcon } from 'brave-ui/components/icons'

export type Type = 'BAT' | 'ETH' | 'BTC' | 'LTC'

export interface Address {
  address: string
  qr: string | null
  type: Type
}

export interface Props {
  onClose: () => void
  id?: string
  isMobile?: boolean
  addresses: Address[]
}

const icons: Record<Type, React.ReactNode> = {
  BAT: <BatColorIcon />,
  ETH: <EthereumColorIcon />,
  BTC: <BitcoinColorIcon />,
  LTC: <LitecoinColorIcon />
}

interface State {
  current?: Type
}

export default class ModalAddFunds extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      current: undefined
    }
  }

  onQR = (type: Type) => {
    this.setState({
      current: type
    })
  }

  getAddress = (address: Address) => {
    const { isMobile } = this.props
    const current = address.type === this.state.current

    return (
      <StyledAddress
        isMobile={!!isMobile}
        key={`address-${address.type}`}
        data-test-id='single-address'
      >
        <StyledHeader>
          <StyledLogo>
            {icons[address.type]}
          </StyledLogo>
          <StyledAddressTitle>
            {getLocale(`title${address.type}`)}
          </StyledAddressTitle>
          <StyledData>
            <StyledWalletAddress>{getLocale('walletAddress')}</StyledWalletAddress>
            <Input value={address.address}/>
          </StyledData>
        </StyledHeader>
        {
          address.qr
          ? (<>
            <StyledQRImageWrapper>
              {
                current
                ? <StyledQRImage src={address.qr} />
                : <StyledShowQR>
                  <StyledQRButton
                    size={'large'}
                    brand={'rewards'}
                    type={'accent'}
                    text={getLocale('addFundsQR')}
                    onClick={this.onQR.bind(this, address.type)}
                  />
                </StyledShowQR>
              }

            </StyledQRImageWrapper>
          </>)
          : null
        }
      </StyledAddress>
    )
  }

  render () {
    const { id, onClose, addresses, isMobile } = this.props

    return (
      <Modal id={id} onClose={onClose} isMobile={isMobile}>
        <StyledWrapper>
          <StyledTitle>{getLocale('addFundsTitle')}</StyledTitle>
          <StyledText>
            {getLocale('addFundsText')}
          </StyledText>
          <StyledAddresses data-test-id='addresses'>
            {
              addresses && addresses.map((address: Address) => this.getAddress(address))
            }
          </StyledAddresses>
          <StyledNote>
            {getLocale('addFundsNote')} <StyledLink href='https://brave.com/faq-payments/#brave-payments' target={'_blank'}>
              {getLocale('addFundsFAQ')}
              </StyledLink>.
          </StyledNote>
        </StyledWrapper>
      </Modal>
    )
  }
}
