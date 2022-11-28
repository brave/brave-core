// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router-dom'

import { NetworkInfo, Swap as SwapInterface } from '@brave/swap-interface'
import '@brave/swap-interface/dist/style.css'

// Utils
import { getLocale } from '$web-common/locale'

// Selectors
import { WalletSelectors } from '../../../common/selectors'
import {
  useSafeWalletSelector,
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'

// Hooks
import { useLib } from '../../../common/hooks'

// Types
import { BraveWallet, WalletAccountType, WalletRoutes } from '../../../constants/types'

// Components
import { BuySendSwapDepositNav } from '../../../components/desktop/buy-send-swap-deposit-nav/buy-send-swap-deposit-nav'

// Adapters
import {
  makeBlockchainToken,
  makeNetworkInfo,
  makeWalletAccount,
  makeSwapService,
  makeSOLSendTransaction,
  makeGetGasPrice,
  makeGetGasPrice1559,
  makeGetERC20ApproveData,
  makeETHSendTransaction,
  makeSwitchNetwork,
  makeSwitchAccount,
  makeGetTokenPrice
} from './adapters'

export const Swap = () => {
  const selectedNetwork = useUnsafeWalletSelector(WalletSelectors.selectedNetwork)
  const selectedAccount = useUnsafeWalletSelector(
    WalletSelectors.selectedAccount
  )
  const accounts: WalletAccountType[] = useUnsafeWalletSelector(WalletSelectors.accounts)
  const networks: BraveWallet.NetworkInfo[] = useUnsafeWalletSelector(WalletSelectors.networkList)
  const defaultFiatCurrency = useSafeWalletSelector(WalletSelectors.defaultFiatCurrency)
  const fullTokenList: BraveWallet.BlockchainToken[] = useUnsafeWalletSelector(
    WalletSelectors.fullTokenList
  )
  const userVisibleTokensInfo: BraveWallet.BlockchainToken[] = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )

  const [supportedNetworks, setSupportedNetworks] = React.useState<NetworkInfo[]>([])

  const tokensList = React.useMemo(() => {
    return [
      ...userVisibleTokensInfo.filter(asset => asset.isErc20),
      ...fullTokenList.filter(
        asset =>
          !userVisibleTokensInfo.some(
            token => token.contractAddress.toLowerCase() === asset.contractAddress.toLowerCase()
          )
      )
    ]
      .filter(asset => !asset.isErc721)
      .map(asset => makeBlockchainToken(asset))
  }, [fullTokenList, userVisibleTokensInfo])

  const {
    getBalanceForChainId,
    getTokenBalanceForChainId,
    sendSolanaSerializedTransaction,
    getSwapService,
    getERC20Allowance,
    sendEthTransaction,
    hasEIP1559Support
  } = useLib()

  const swapServiceMojo = getSwapService()

  React.useEffect(() => {
    ;(async () => {
      const results = await Promise.all(
        networks.map(async e => (await swapServiceMojo.isSwapSupported(e.chainId)).result)
      )

      const result = networks.filter((_, index) => results[index]).map(e => makeNetworkInfo(e))
      setSupportedNetworks(result)
    })()
  }, [networks])

  // Memos
  const walletAccounts = React.useMemo(() => {
    return accounts.map(account => makeWalletAccount(account))
  }, [accounts])

  const ethWalletAdapter = React.useMemo(() => {
    return {
      getGasPrice: makeGetGasPrice(),
      getGasPrice1559: makeGetGasPrice1559(),
      getERC20Allowance,
      getERC20ApproveData: makeGetERC20ApproveData(),
      sendTransaction: makeETHSendTransaction(
        sendEthTransaction,
        !!selectedNetwork && !!selectedAccount && hasEIP1559Support(selectedAccount, selectedNetwork)
      )
    }
  }, [selectedAccount, selectedNetwork])

  const solWalletAdapter = React.useMemo(() => {
    return {
      sendTransaction: makeSOLSendTransaction(sendSolanaSerializedTransaction)
    }
  }, [])

  let history = useHistory()
  const goBack = React.useCallback(() => {
    history.push(WalletRoutes.Portfolio)
  }, [history])

  return (
    <div>
      <BuySendSwapDepositNav isTab={true} />
      {selectedNetwork && selectedAccount && (
        <SwapInterface
          getLocale={getLocale}
          network={makeNetworkInfo(selectedNetwork)}
          account={makeWalletAccount(selectedAccount)}
          walletAccounts={walletAccounts}
          switchAccount={makeSwitchAccount()}
          switchNetwork={makeSwitchNetwork()}
          getBalance={getBalanceForChainId}
          getTokenBalance={getTokenBalanceForChainId}
          // // FIXME - remove isSwapSupported()
          swapService={makeSwapService()}
          // // FIXME - implement mojo method to query available 0x exchanges
          // // This feature may be disabled until then.
          exchanges={[]}
          // // FIXME - remove getNetworkFeeEstimate, since chain-specific methods are
          // // already available in the wallet adapters. This is a stub to make tsc happy.
          getNetworkFeeEstimate={async () => ({ gasFee: '' })}
          assetsList={tokensList}
          solWalletAdapter={solWalletAdapter}
          ethWalletAdapter={ethWalletAdapter}
          getTokenPrice={makeGetTokenPrice(defaultFiatCurrency)}
          supportedNetworks={supportedNetworks}
          defaultBaseCurrency={defaultFiatCurrency}
          routeBackToWallet={goBack}
          isWalletConnected={true}
          isReady={!!selectedNetwork}
        />
      )}
    </div>
  )
}

export default Swap
