// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'
import { DialogProps } from '@brave/leo/react/dialog'
import { InputEventDetail } from '@brave/leo/react/input'
import Button from '@brave/leo/react/button'

// Queries
import { useAccountsQuery } from '../../../../../common/slices/api.slice.extra'
import {
  useAddAccountMutation,
  useGetNetworkQuery
} from '../../../../../common/slices/api.slice'

// Selectors
import {
  useSafeUISelector //
} from '../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../common/selectors'

// Types
import { MeldCryptoCurrency, BraveWallet } from '../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../common/locale'
import { suggestNewAccountName } from '../../../../../utils/address-utils'
import { keyringIdForNewAccount } from '../../../../../utils/account-utils'
import { getMeldTokensCoinType } from '../../../../../utils/meld_utils'

// Components
import {
  BottomSheet //
} from '../../../../../components/shared/bottom_sheet/bottom_sheet'
import {
  CreateNetworkIcon //
} from '../../../../../components/shared/create-network-icon'

// Styled Components
import { Input, Dialog } from './create_account.style'
import { DialogTitle } from '../shared/style'
import { Column, Row, Text } from '../../../../../components/shared/style'

interface Props extends DialogProps {
  isOpen: boolean
  token?: MeldCryptoCurrency
  onSelectToken: (asset: MeldCryptoCurrency) => void
  onClose: () => void
}

export const CreateAccount = (props: Props) => {
  const { token, onSelectToken, onClose, isOpen, ...rest } = props

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // State
  const [accountName, setAccountName] = React.useState<string>('')

  // Queries
  const { accounts } = useAccountsQuery()
  const { data: network } = useGetNetworkQuery(
    token?.chainId
      ? {
          chainId: token.chainId,
          coin: getMeldTokensCoinType(token)
        }
      : skipToken
  )

  // Computed
  const suggestedAccountName = network
    ? suggestNewAccountName(accounts, network)
    : ''

  // Mutations
  const [addAccount] = useAddAccountMutation()

  // Methods
  const onClickCreateAccount = React.useCallback(async () => {
    if (!network || !token) {
      return
    }
    try {
      const account = await addAccount({
        coin: network.coin,
        keyringId: keyringIdForNewAccount(network.coin, network.chainId),
        accountName: suggestedAccountName
      }).unwrap()

      if (account) {
        onSelectToken(token)
      }
    } catch (error) {
      console.log(error)
    }
  }, [addAccount, network, suggestedAccountName, onSelectToken, token])

  const handleAccountNameChanged = (detail: InputEventDetail) => {
    setAccountName(detail.value)
  }

  const handleKeyDown = React.useCallback(
    (detail: InputEventDetail) => {
      if ((detail.innerEvent as unknown as KeyboardEvent).key === 'Enter') {
        onClickCreateAccount()
      }
    },
    [onClickCreateAccount]
  )

  // Effects
  React.useEffect(() => {
    setAccountName(suggestedAccountName)
  }, [suggestedAccountName])

  // Memos
  const createAccountContent = React.useMemo(() => {
    return (
      <>
        <DialogTitle slot='title'>
          {getLocale('braveWalletCreateAccountButton')}
        </DialogTitle>
        <Column
          gap='16px'
          margin='0px 0px 46px 0px'
        >
          <CreateNetworkIcon
            network={network}
            size='massive'
          />
          <Text
            textSize='22px'
            textColor='primary'
            isBold={true}
          >
            {token
              ? getLocale('braveWalletCreateAccountToBuyTitle').replace(
                  '$1',
                  token.name ?? ''
                )
              : ''}
          </Text>
          <Text
            textSize='14px'
            textColor='tertiary'
            isBold={false}
          >
            {token
              ? getLocale('braveWalletCreateAccountToBuyDescription')
                  .replace('$1', token.currencyCode ?? '')
                  .replace('$2', token.name ?? '')
              : ''}
          </Text>
        </Column>
        <Row margin='0px 0px 46px 0px'>
          <Input
            value={accountName}
            placeholder={getLocale('braveWalletAddAccountPlaceholder')}
            onInput={handleAccountNameChanged}
            onKeyDown={handleKeyDown}
            maxlength={BraveWallet.ACCOUNT_NAME_MAX_CHARACTER_LENGTH}
          >
            <Text
              textSize='12px'
              textColor='primary'
              isBold={true}
            >
              {getLocale('braveWalletAccountName')}
            </Text>
          </Input>
        </Row>
        <Row gap='16px'>
          <Button
            kind='outline'
            onClick={onClose}
          >
            {getLocale('braveWalletButtonCancel')}
          </Button>
          <Button
            kind='filled'
            onClick={onClickCreateAccount}
            isDisabled={accountName === ''}
          >
            {getLocale('braveWalletCreateAccountButton')}
          </Button>
        </Row>
      </>
    )
  }, [
    network,
    token,
    accountName,
    onClose,
    onClickCreateAccount,
    handleKeyDown
  ])

  if (isPanel) {
    return (
      <BottomSheet
        onClose={onClose}
        isOpen={isOpen}
      >
        <Column
          fullWidth={true}
          padding='0px 16px'
          height='90vh'
        >
          {createAccountContent}
        </Column>
      </BottomSheet>
    )
  }

  return (
    <Dialog
      isOpen={isOpen}
      onClose={onClose}
      showClose
      backdropClickCloses={false}
      {...rest}
    >
      {createAccountContent}
    </Dialog>
  )
}
