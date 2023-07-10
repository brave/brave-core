// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create } from 'ethereum-blockies'

// Queries
import { useSelectedAccountQuery } from '../../../../../../common/slices/api.slice.extra'

// Utils
import { reduceAddress } from '../../../../../../utils/reduce-address'
import { getLocale } from '../../../../../../../common/locale'

// Styled Components
import {
  AccountCircle,
  Button,
  ButtonIcon
} from './connect-wallet-button.style'
import {
  Text,
  HorizontalSpacer,
  HiddenResponsiveRow
} from '../../shared-swap.styles'

interface Props {
  onClick: () => void
}

export const ConnectWalletButton = (props: Props) => {
  const { onClick } = props

  // Selectors
  const { data: selectedAccount } = useSelectedAccountQuery()

  // Memos
  const accountOrb: string | undefined = React.useMemo(() => {
    if (!selectedAccount?.address) {
      return
    }

    return create({
      seed: selectedAccount.address.toLowerCase() || '',
      size: 8,
      scale: 16
    }).toDataURL()
  }, [selectedAccount])

  return (
    <Button onClick={onClick} isConnected={selectedAccount !== undefined}>
      {selectedAccount ? (
        <>
          {accountOrb && <AccountCircle orb={accountOrb} />}{' '}
          <HiddenResponsiveRow>
            <Text textSize='14px' textColor='text01' isBold={true}>
              {selectedAccount.name}
            </Text>
            <HorizontalSpacer size={4} />
          </HiddenResponsiveRow>
          <Text
            textSize='14px'
            textColor='text02'
            isBold={true}
            responsiveTextSize='12px'
          >
            {selectedAccount.address ? reduceAddress(selectedAccount.address) : ' '}
          </Text>
          <HorizontalSpacer size={7} />
          <ButtonIcon name='carat-down' size={16} />
        </>
      ) : (
        getLocale('braveSwapConnectWallet')
      )}
    </Button>
  )
}
