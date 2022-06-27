// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Redux
import { useSelector, useDispatch } from 'react-redux'
import { WalletActions } from '../../../common/actions'
import { PanelActions } from '../../../panel/actions'

// Types
import { BraveWallet, WalletState } from '../../../constants/types'

// Components
import { NavButton } from '../../extension'
import { LockPanel } from '../../extension/lock-panel'

// Utils
import { getLocale } from '../../../../common/locale'
import { suggestNewAccountName } from '../../../utils/address-utils'

// Styled Components
import {
  StyledWrapper,
  Description,
  ButtonRow
} from './style'

export interface Props {
  isPanel?: boolean
  network?: BraveWallet.NetworkInfo
  prevNetwork?: BraveWallet.NetworkInfo | undefined
  onCancel?: () => void
}

export const CreateAccountTab = ({
  isPanel,
  prevNetwork,
  network,
  onCancel
}: Props) => {
  // redux
  const {
    networkList,
    selectedNetwork,
    accounts,
    isWalletLocked
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  const dispatch = useDispatch()

  // state
  const [showUnlock, setShowUnlock] = React.useState(false)

  // memos
  const accountNetwork = React.useMemo(() => {
    return network || selectedNetwork
  }, [network, selectedNetwork])

  const suggestedAccountName = React.useMemo((): string => {
    return suggestNewAccountName(accounts, accountNetwork)
  }, [accounts, accountNetwork])

  // methods
  const onCancelCreateAccount = React.useCallback(() => {
    if (onCancel) {
      return onCancel()
    }
    dispatch(WalletActions.selectNetwork(prevNetwork ?? networkList[0]))
    if (isPanel) {
      dispatch(PanelActions.navigateTo('main'))
    }
  }, [onCancel, prevNetwork, networkList, isPanel])

  const onCreateAccount = React.useCallback(() => {
    // unlock needed to create accounts
    if (isWalletLocked && !showUnlock) {
      return setShowUnlock(true)
    }

    if (selectedNetwork.coin === BraveWallet.CoinType.FIL) {
      dispatch(WalletActions.addFilecoinAccount({
        accountName: suggestedAccountName,
        network: accountNetwork.chainId
      }))
    } else {
      dispatch(WalletActions.addAccount({
        accountName: suggestedAccountName,
        coin: accountNetwork.coin
      }))
    }

    if (isPanel) {
      dispatch(PanelActions.navigateTo('main'))
    }
  }, [accountNetwork, suggestedAccountName, isPanel, isWalletLocked, showUnlock])

  const handleUnlockAttempt = React.useCallback((password: string): void => {
    dispatch(WalletActions.unlockWallet({ password }))
    dispatch(WalletActions.addAccount({
      accountName: suggestedAccountName,
      coin: accountNetwork.coin
    }))
  }, [suggestedAccountName, accountNetwork])

  // effects
  React.useEffect(() => {
    // hide unlock screen on unlock success
    if (!isWalletLocked && showUnlock) {
      setShowUnlock(false)
    }
  }, [isWalletLocked, showUnlock])

  // render
  if (isWalletLocked && showUnlock) {
    return <StyledWrapper>
      <Description style={{ fontSize: 16 }}>
        {
          'Unlock needed to create an account'
          // getLocale('braveWalletCreateAccountDescription')
        }
      </Description>
      <LockPanel
        hideBackground
        onSubmit={handleUnlockAttempt}
      />
    </StyledWrapper>
  }

  return (
    <StyledWrapper>
      <Description>{getLocale('braveWalletCreateAccountDescription').replace('$1', accountNetwork.symbolName)}</Description>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          onSubmit={onCancelCreateAccount}
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
