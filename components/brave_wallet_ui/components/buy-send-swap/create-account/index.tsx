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

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  Description,
  ButtonRow
} from './style'

export interface Props {
  isPanel?: boolean
  prevNetwork: BraveWallet.NetworkInfo | undefined
}

function CreateAccountTab (props: Props) {
  const { isPanel, prevNetwork } = props

  // redux
  const {
    networkList,
    selectedNetwork,
    accounts
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  const dispatch = useDispatch()

  const suggestedAccountName = React.useMemo((): string => {
    const accountTypeLength = accounts.filter((account) => account.coin === selectedNetwork.coin).length + 1
    return `${selectedNetwork.symbolName} ${getLocale('braveWalletAccount')} ${accountTypeLength}`
  }, [selectedNetwork, accounts])

  const onCancelCreateAccount = React.useCallback(() => {
    dispatch(WalletActions.selectNetwork(prevNetwork ?? networkList[0]))
    if (isPanel) {
      dispatch(PanelActions.navigateTo('main'))
    }
  }, [prevNetwork, networkList, isPanel])

  const onCreateAccount = React.useCallback(() => {
    dispatch(WalletActions.addAccount({ accountName: suggestedAccountName, coin: selectedNetwork.coin }))
    if (isPanel) {
      dispatch(PanelActions.navigateTo('main'))
    }
  }, [selectedNetwork, suggestedAccountName, isPanel])

  return (
    <StyledWrapper>
      <Description>{getLocale('braveWalletCreateAccountDescription').replace('$1', selectedNetwork.symbolName)}</Description>
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
