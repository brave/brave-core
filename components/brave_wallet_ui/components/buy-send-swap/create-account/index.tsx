// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Redux
import { useSafeWalletSelector } from '../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../common/selectors'
import {
  useAddAccountMutation,
  useGetNetworkQuery
} from '../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../constants/types'

// Components
import { NavButton } from '../../extension/buttons/nav-button/index'

// Utils
import { getLocale } from '../../../../common/locale'
import { suggestNewAccountName } from '../../../utils/address-utils'
import { keyringIdForNewAccount } from '../../../utils/account-utils'

// Styled Components
import { StyledWrapper, Description, ButtonRow } from './style'
import { useAccountsQuery } from '../../../common/slices/api.slice.extra'

export interface Props {
  network: Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
  onCreated?: (account: BraveWallet.AccountInfo) => void
  onCancel: () => void
}

export const CreateAccountTab = ({
  network: accountNetwork,
  onCreated,
  onCancel
}: Props) => {
  // redux
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)

  // queries
  const { accounts } = useAccountsQuery()
  const { data: network } = useGetNetworkQuery({
    chainId: accountNetwork.chainId,
    coin: accountNetwork.coin
  })

  // mutations
  const [addAccount] = useAddAccountMutation()

  // state
  const [showUnlock, setShowUnlock] = React.useState<boolean>(false)

  const suggestedAccountName = network
    ? suggestNewAccountName(accounts, network)
    : ''

  const onCreateAccount = React.useCallback(async () => {
    // unlock needed to create accounts
    if (isWalletLocked && !showUnlock) {
      return setShowUnlock(true)
    }

    try {
      const account = await addAccount({
        coin: accountNetwork.coin,
        keyringId: keyringIdForNewAccount(
          accountNetwork.coin,
          accountNetwork.chainId
        ),
        accountName: suggestedAccountName
      }).unwrap()

      if (account && onCreated) {
        onCreated(account)
      }
    } catch (error) {
      console.log(error)
    }
  }, [
    isWalletLocked,
    showUnlock,
    accountNetwork,
    suggestedAccountName,
    onCreated
  ])

  // effects
  React.useEffect(() => {
    // hide unlock screen on unlock success
    if (!isWalletLocked && showUnlock) {
      setShowUnlock(false)
    }
  }, [isWalletLocked, showUnlock])

  // render
  return (
    <StyledWrapper>
      <Description>
        {network
          ? getLocale('braveWalletCreateAccountDescription').replace(
              '$1',
              network.symbolName
            )
          : ''}
      </Description>

      <ButtonRow>
        <NavButton
          buttonType='secondary'
          onSubmit={onCancel}
          text={getLocale('braveWalletCreateAccountNo')}
        />
        <NavButton
          buttonType='primary'
          onSubmit={onCreateAccount}
          text={getLocale('braveWalletCreateAccountYes')}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default CreateAccountTab
