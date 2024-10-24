// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Slices
import { useMakeAccountShieldedMutation } from '../../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'

// Components
import { PopupModal } from '../../popup-modals/index'
import {
  CreateAccountIcon //
} from '../../../shared/create-account-icon/create-account-icon'

// Styles
import {
  AccountRow,
  ShieldIconWrapper,
  ShieldIcon
} from './shield_zcash_account.style'
import { Column, Text, Row, LeoSquaredButton } from '../../../shared/style'

interface Props {
  account: BraveWallet.AccountInfo
  onClose: () => void
}

export const ShieldZCashAccountModal = (props: Props) => {
  const { account, onClose } = props

  // State
  const [isShielding, setIsShielding] = React.useState<boolean>(false)

  // mutations
  const [shieldAccount] = useMakeAccountShieldedMutation()

  const onShieldAccount = React.useCallback(async () => {
    if (!account.accountId) {
      return
    }
    setIsShielding(true)
    await shieldAccount(account.accountId)
    setIsShielding(false)
    onClose()
  }, [shieldAccount, account, onClose])

  return (
    <PopupModal
      title={getLocale('braveWalletSwitchToShieldedAccount')}
      onClose={onClose}
      width='520px'
      headerPaddingHorizontal='32px'
      headerPaddingVertical='32px'
    >
      <Column
        gap='24px'
        padding='8px 32px'
      >
        <AccountRow
          width='unset'
          padding='8px 30px 8px 8px'
        >
          <ShieldIconWrapper>
            <ShieldIcon />
          </ShieldIconWrapper>
          <CreateAccountIcon
            account={account}
            size='huge'
            marginRight={16}
          />
          <Column alignItems='flex-start'>
            <Text
              textSize='14px'
              isBold={true}
              textColor='primary'
              textAlign='left'
            >
              {account.name}
            </Text>
            <Text
              textSize='12px'
              isBold={false}
              textColor='secondary'
              textAlign='left'
            >
              ZCash
            </Text>
          </Column>
        </AccountRow>
        <Text
          textSize='14px'
          isBold={false}
          textColor='primary'
          textAlign='left'
        >
          {getLocale('braveWalletAccountNotShieldedDescription')}
        </Text>
        <Text
          textSize='14px'
          isBold={true}
          textColor='primary'
          textAlign='left'
        >
          {getLocale('braveWalletAccountShieldedDescription')}
        </Text>
      </Column>
      <Row
        gap='16px'
        padding='32px'
      >
        <LeoSquaredButton
          onClick={onClose}
          kind='outline'
        >
          {getLocale('braveWalletButtonCancel')}
        </LeoSquaredButton>
        <LeoSquaredButton
          onClick={onShieldAccount}
          disabled={isShielding}
        >
          {getLocale('braveWalletShieldAccount')}
        </LeoSquaredButton>
      </Row>
    </PopupModal>
  )
}
