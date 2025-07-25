// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */
import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useHistory, useLocation } from 'react-router'

// Selectors
import {
  useSafeUISelector,
  useSafeWalletSelector,
} from '../../../../common/hooks/use-safe-selector'
import { UISelectors, WalletSelectors } from '../../../../common/selectors'

// Types
import {
  SendPageTabHashes,
  WalletRoutes,
  CoinTypesMap,
  BraveWallet,
  BaseTransactionParams,
  AmountValidationErrorType,
} from '../../../../constants/types'

// Constants
import { MAX_ZCASH_MEMO_LENGTH } from '../constants/magics'

// Utils
import { getLocale } from '../../../../../common/locale'
import Amount from '../../../../utils/amount'
import { getBalance } from '../../../../utils/balance-utils'
import { isValidFilAddress } from '../../../../utils/address-utils'
import { makeSendRoute } from '../../../../utils/routes-utils'
import {
  selectAllVisibleUserAssetsFromQueryResult, //
} from '../../../../common/slices/entities/blockchain-token.entity'
import {
  getDominantColorFromImageURL, //
} from '../../../../utils/style.utils'

// Hooks
import {
  useScopedBalanceUpdater, //
} from '../../../../common/hooks/use-scoped-balance-updater'
import { useModal } from '../../../../common/hooks/useOnClickOutside'
import { useQuery } from '../../../../common/hooks/use-query'
import {
  useGetUserTokensRegistryQuery,
  useSendSPLTransferMutation,
  useSendERC20TransferMutation,
  useSendERC721TransferFromMutation,
  useSendETHFilForwarderTransferMutation,
  useGetVisibleNetworksQuery,
  useSendEvmTransactionMutation,
  useSendSolTransactionMutation,
  useSendFilTransactionMutation,
  useSendBtcTransactionMutation,
  useSendZecTransactionMutation,
  useSendCardanoTransactionMutation,
  useGetZCashTransactionTypeQuery,
} from '../../../../common/slices/api.slice'
import {
  useAccountFromAddressQuery, //
} from '../../../../common/slices/api.slice.extra'
import {
  useIsAccountSyncing, //
} from '../../../../common/hooks/use_is_account_syncing'

// Styled Components
import { InputRow, ToText, ToRow, ShieldingFundsAlert } from './send.style'
import {
  ToSectionWrapper,
  ReviewButtonRow,
} from '../../composer_ui/shared_composer.style'
import {
  Column,
  LeoSquaredButton,
  Row,
} from '../../../../components/shared/style'

// Components
import {
  SelectAddressModal, //
} from '../components/select_address_modal/select_address_modal'
import {
  SelectTokenModal, //
} from '../../composer_ui/select_token_modal/select_token_modal'
import {
  WalletPageWrapper, //
} from '../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import { FromAsset } from '../../composer_ui/from_asset/from_asset'
import {
  PanelActionHeader, //
} from '../../../../components/desktop/card-headers/panel-action-header'
import {
  OrdinalsWarningMessage, //
} from '../components/ordinals-warning-message/ordinals-warning-message'
import {
  SelectAddressButton, //
} from '../../composer_ui/select_address_button/select_address_button'
import { AddMemo } from '../components/add_memo/add_memo'

export const SendScreen = React.memo(() => {
  // routing
  const query = useQuery()
  const history = useHistory()
  const { hash } = useLocation()
  const selectedSendOption = (hash as SendPageTabHashes) || '#token'
  const accountIdFromParams = query.get('account') ?? undefined
  const chainIdFromParams = query.get('chainId') ?? undefined
  const contractOrSymbolFromParams = query.get('token') ?? undefined
  const toAddressOrUrl = query.get('recipient') ?? ''

  const { account: accountFromParams } =
    useAccountFromAddressQuery(accountIdFromParams)

  const { data: networks = [] } = useGetVisibleNetworksQuery()
  const networkFromParams = React.useMemo(
    () => networks.find((network) => network.chainId === chainIdFromParams),
    [networks, chainIdFromParams],
  )

  // State
  const [sendAmount, setSendAmount] = React.useState<string>(
    selectedSendOption === '#nft' ? '1' : '',
  )
  const [sendingMaxAmount, setSendingMaxAmount] = React.useState<boolean>(false)
  const [resolvedDomainAddress, setResolvedDomainAddress] =
    React.useState<string>('')
  const [isWarningAcknowledged, setIsWarningAcknowledged] =
    React.useState<boolean>(false)
  const [memoText, setMemoText] = React.useState<string>('')

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isAndroid = useSafeUISelector(UISelectors.isAndroid)
  const isAndroidOrPanel = isAndroid || isPanel
  const isZCashShieldedTransactionsEnabled = useSafeWalletSelector(
    WalletSelectors.isZCashShieldedTransactionsEnabled,
  )

  // Mutations
  const [sendSPLTransfer] = useSendSPLTransferMutation()
  const [sendEvmTransaction] = useSendEvmTransactionMutation()
  const [sendSolTransaction] = useSendSolTransactionMutation()
  const [sendFilTransaction] = useSendFilTransactionMutation()
  const [sendBtcTransaction] = useSendBtcTransactionMutation()
  const [sendZecTransaction] = useSendZecTransactionMutation()
  const [sendCardanoTransaction] = useSendCardanoTransactionMutation()
  const [sendERC20Transfer] = useSendERC20TransferMutation()
  const [sendERC721TransferFrom] = useSendERC721TransferFromMutation()
  const [sendETHFilForwarderTransfer] = useSendETHFilForwarderTransferMutation()

  // Queries
  const { userVisibleTokensInfo } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: (result) => ({
      userVisibleTokensInfo: selectAllVisibleUserAssetsFromQueryResult(result),
    }),
  })

  const {
    data: getZCashTransactionTypeResult = { txType: null, error: null },
  } = useGetZCashTransactionTypeQuery(
    networkFromParams?.coin === BraveWallet.CoinType.ZEC
      && accountFromParams
      && toAddressOrUrl
      ? {
          chainId: networkFromParams.chainId,
          accountId: accountFromParams.accountId,
          useShieldedPool: query.get('isShielded') === 'true',
          address: toAddressOrUrl,
        }
      : skipToken,
  )

  const tokenFromParams = React.useMemo(() => {
    if (!networkFromParams) {
      return
    }

    if (!contractOrSymbolFromParams) {
      return
    }

    const tokenId = query.get('tokenId')

    const isShielded = query.get('isShielded') === 'true'

    return userVisibleTokensInfo.find((token) =>
      tokenId
        ? token.chainId === networkFromParams.chainId
          && token.contractAddress.toLowerCase()
            === contractOrSymbolFromParams.toLowerCase()
          && token.tokenId === tokenId
          && token.isShielded === isShielded
        : (token.chainId === networkFromParams.chainId
            && token.contractAddress.toLowerCase()
              === contractOrSymbolFromParams.toLowerCase())
          || (token.chainId === networkFromParams.chainId
            && token.contractAddress === ''
            && token.symbol.toLowerCase()
              === contractOrSymbolFromParams.toLowerCase()
            && token.isShielded === isShielded),
    )
  }, [
    userVisibleTokensInfo,
    query,
    networkFromParams,
    contractOrSymbolFromParams,
  ])

  const { data: tokenBalancesRegistry, isLoading: isLoadingBalances } =
    useScopedBalanceUpdater(
      accountFromParams && networkFromParams && tokenFromParams
        ? {
            network: networkFromParams,
            accounts: [accountFromParams],
            tokens: [tokenFromParams],
          }
        : skipToken,
    )

  const isAccountSyncing = useIsAccountSyncing(accountFromParams?.accountId)
  const isShieldingFunds =
    tokenFromParams
    && tokenFromParams.coin === BraveWallet.CoinType.ZEC
    && getZCashTransactionTypeResult.txType
      === BraveWallet.ZCashTxType.kShielding

  // memos & computed
  const sendAmountValidationError: AmountValidationErrorType | undefined =
    React.useMemo(() => {
      if (!sendAmount || !tokenFromParams) {
        return
      }

      // extract BigNumber object wrapped by Amount
      const amountBN = ethToWeiAmount(sendAmount, tokenFromParams).value

      const amountDP = amountBN && amountBN.decimalPlaces()
      return amountDP && amountDP > 0 ? 'fromAmountDecimalsOverflow' : undefined
    }, [sendAmount, tokenFromParams])

  const sendAssetBalance =
    !accountFromParams || !tokenFromParams || !tokenBalancesRegistry
      ? ''
      : getBalance(
          accountFromParams.accountId,
          tokenFromParams,
          tokenBalancesRegistry,
        )

  const insufficientFundsError = React.useMemo((): boolean => {
    if (!tokenFromParams) {
      return false
    }

    const amountWei = new Amount(sendAmount).multiplyByDecimals(
      tokenFromParams.decimals,
    )

    if (amountWei.isZero()) {
      return false
    }

    return amountWei.gt(sendAssetBalance)
  }, [sendAssetBalance, sendAmount, tokenFromParams])

  const tokenColor = React.useMemo(() => {
    return getDominantColorFromImageURL(tokenFromParams?.logo ?? '')
  }, [tokenFromParams?.logo])

  const needsAccountSelected =
    accountIdFromParams === undefined
    && contractOrSymbolFromParams !== undefined
    && chainIdFromParams !== undefined

  // Methods
  const selectSendAsset = React.useCallback(
    (asset: BraveWallet.BlockchainToken, account?: BraveWallet.AccountInfo) => {
      const isNftTab = asset.isErc721 || asset.isNft
      if (isNftTab) {
        setSendAmount('1')
      } else {
        setSendAmount('')
      }

      if (toAddressOrUrl && needsAccountSelected && account) {
        history.replace(makeSendRoute(asset, account, toAddressOrUrl))
        return
      }

      if (account) {
        history.replace(makeSendRoute(asset, account))
      }
    },
    [history, needsAccountSelected, toAddressOrUrl],
  )

  const resetSendFields = React.useCallback(
    (option?: SendPageTabHashes) => {
      setSendAmount('')

      if (option) {
        history.push(`${WalletRoutes.Send}${option}`)
      } else {
        history.push(WalletRoutes.Send)
      }
    },
    [history],
  )

  const submitSend = React.useCallback(async () => {
    if (!tokenFromParams) {
      console.log('Failed to submit Send transaction: no send asset selected')
      return
    }

    if (!accountFromParams) {
      console.log('Failed to submit Send transaction: no account selected')
      return
    }

    if (!networkFromParams) {
      console.log('Failed to submit Send transaction: no network selected')
      return
    }

    const fromAccount: BaseTransactionParams['fromAccount'] = {
      accountId: accountFromParams.accountId,
      address: accountFromParams.address,
      hardware: accountFromParams.hardware,
    }

    const toAddress =
      resolvedDomainAddress !== '' ? resolvedDomainAddress : toAddressOrUrl

    switch (fromAccount.accountId.coin) {
      case BraveWallet.CoinType.BTC: {
        await sendBtcTransaction({
          network: networkFromParams,
          fromAccount,
          to: toAddress,
          sendingMaxAmount,
          value: new Amount(sendAmount)
            .multiplyByDecimals(tokenFromParams.decimals)
            .toHex(),
        })
        resetSendFields()
        return
      }

      case BraveWallet.CoinType.ETH: {
        if (tokenFromParams.isErc20) {
          await sendERC20Transfer({
            network: networkFromParams,
            fromAccount,
            to: toAddress,
            value: ethToWeiAmount(sendAmount, tokenFromParams).toHex(),
            gasLimit: '',
            contractAddress: tokenFromParams.contractAddress,
            data: [],
          })
          resetSendFields()
          return
        }

        if (tokenFromParams.isErc721) {
          await sendERC721TransferFrom({
            network: networkFromParams,
            fromAccount,
            to: toAddress,
            value: '',
            gasLimit: '',
            contractAddress: tokenFromParams.contractAddress,
            tokenId: tokenFromParams.tokenId ?? '',
            data: [],
          })
          resetSendFields()
          return
        }

        if (
          (tokenFromParams.chainId
            === BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID
            || tokenFromParams.chainId
              === BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID)
          && isValidFilAddress(toAddress)
        ) {
          await sendETHFilForwarderTransfer({
            network: networkFromParams,
            fromAccount,
            to: toAddress,
            value: ethToWeiAmount(sendAmount, tokenFromParams).toHex(),
            gasLimit: '',
            contractAddress: '0x2b3ef6906429b580b7b2080de5ca893bc282c225',
            data: [],
          })
          resetSendFields()
          return
        }

        await sendEvmTransaction({
          network: networkFromParams,
          fromAccount,
          to: toAddress,
          value: new Amount(sendAmount)
            .multiplyByDecimals(tokenFromParams.decimals)
            .toHex(),
          gasLimit: '',
          data: [],
        })
        resetSendFields()
        return
      }

      case BraveWallet.CoinType.FIL: {
        await sendFilTransaction({
          network: networkFromParams,
          fromAccount,
          to: toAddress,
          value: new Amount(sendAmount)
            .multiplyByDecimals(tokenFromParams.decimals)
            .format(),
        })
        resetSendFields()
        return
      }

      case BraveWallet.CoinType.SOL: {
        if (
          tokenFromParams.contractAddress !== ''
          && !tokenFromParams.isErc20
          && !tokenFromParams.isErc721
        ) {
          await sendSPLTransfer({
            network: networkFromParams,
            fromAccount,
            to: toAddress,
            value: !tokenFromParams.isNft
              ? new Amount(sendAmount)
                  .multiplyByDecimals(tokenFromParams.decimals)
                  .toHex()
              : new Amount(sendAmount).toHex(),
            splTokenMintAddress: tokenFromParams.contractAddress,
            decimals: tokenFromParams.decimals,
            isCompressedNft: tokenFromParams.isCompressed,
          })
          resetSendFields()
          return
        }

        await sendSolTransaction({
          network: networkFromParams,
          fromAccount,
          to: toAddress,
          value: new Amount(sendAmount)
            .multiplyByDecimals(tokenFromParams.decimals)
            .toHex(),
        })
        resetSendFields()
        return
      }

      case BraveWallet.CoinType.ZEC: {
        const memoArray =
          memoText !== '' ? new TextEncoder().encode(memoText) : undefined
        await sendZecTransaction({
          useShieldedPool: tokenFromParams.isShielded,
          network: networkFromParams,
          fromAccount,
          to: toAddress,
          sendingMaxAmount,
          value: new Amount(sendAmount)
            .multiplyByDecimals(tokenFromParams.decimals)
            .toHex(),
          memo: memoArray ? Array.from(memoArray) : undefined,
        })
        resetSendFields()
        return
      }

      case BraveWallet.CoinType.ADA: {
        await sendCardanoTransaction({
          network: networkFromParams,
          fromAccount,
          to: toAddress,
          sendingMaxAmount,
          value: new Amount(sendAmount)
            .multiplyByDecimals(tokenFromParams.decimals)
            .toHex(),
        })
        resetSendFields()
      }
    }
  }, [
    tokenFromParams,
    accountFromParams,
    networkFromParams,
    toAddressOrUrl,
    sendBtcTransaction,
    sendingMaxAmount,
    sendAmount,
    resolvedDomainAddress,
    memoText,
    resetSendFields,
    sendEvmTransaction,
    sendERC20Transfer,
    sendERC721TransferFrom,
    sendETHFilForwarderTransfer,
    sendFilTransaction,
    sendSolTransaction,
    sendSPLTransfer,
    sendZecTransaction,
    sendCardanoTransaction,
  ])

  const handleFromAssetValueChange = React.useCallback(
    (value: string, maxValue: boolean) => {
      setSendAmount(value)
      setSendingMaxAmount(maxValue)
    },
    [],
  )

  const onSelectSendOption = React.useCallback(
    (option: SendPageTabHashes) => {
      resetSendFields(option)
    },
    [resetSendFields],
  )

  const onChangeToAddressOrUrl = React.useCallback(
    (addressOrUrl: string) => {
      if (tokenFromParams) {
        history.replace(
          makeSendRoute(tokenFromParams, accountFromParams, addressOrUrl),
        )
      }
    },
    [history, tokenFromParams, accountFromParams],
  )

  // Modals
  const {
    closeModal: closeSelectTokenModal,
    openModal: openSelectTokenModal,
    ref: selectTokenModalRef,
    isModalShown: showSelectTokenModal,
  } = useModal()

  const {
    closeModal: closeSelectAddressModal,
    openModal: openSelectAddressModal,
    ref: selectAddressModalRef,
    isModalShown: showSelectAddressModal,
  } = useModal()

  // Effects
  React.useEffect(() => {
    if (needsAccountSelected) {
      openSelectTokenModal()
    }
  }, [needsAccountSelected, openSelectTokenModal])

  // render
  return (
    <>
      <WalletPageWrapper
        wrapContentInBox={true}
        noCardPadding={true}
        hideNav={isAndroidOrPanel}
        cardHeader={
          isAndroidOrPanel ? (
            <PanelActionHeader
              title={getLocale('braveWalletSend')}
              expandRoute={WalletRoutes.Send}
            />
          ) : undefined
        }
      >
        <Column
          fullWidth={true}
          fullHeight={true}
        >
          <FromAsset
            onInputChange={handleFromAssetValueChange}
            onClickSelectToken={openSelectTokenModal}
            hasInputError={insufficientFundsError}
            inputValue={sendAmount}
            account={accountFromParams}
            network={networkFromParams}
            token={tokenFromParams}
            isLoadingBalances={isLoadingBalances}
            tokenBalancesRegistry={tokenBalancesRegistry}
          />
          <ToSectionWrapper
            fullWidth={true}
            fullHeight={true}
            justifyContent='flex-start'
            tokenColor={tokenColor}
          >
            <Column
              fullWidth={true}
              fullHeight={true}
              justifyContent='space-between'
              alignItems='center'
              padding='32px 0px 0px 0px'
            >
              <Column
                fullWidth={true}
                margin='0px 0px 16px 0px'
                justifyContent='space-between'
              >
                <ToRow
                  width='100%'
                  alignItems='center'
                  justifyContent='flex-start'
                  marginBottom={10}
                >
                  <ToText
                    textSize='14px'
                    isBold={false}
                  >
                    {getLocale('braveWalletSwapTo')}
                  </ToText>
                </ToRow>
                <InputRow
                  width='100%'
                  justifyContent='flex-start'
                >
                  <SelectAddressButton
                    onClick={openSelectAddressModal}
                    isDisabled={!tokenFromParams}
                    toAddressOrUrl={toAddressOrUrl}
                  />
                </InputRow>
                {tokenFromParams?.coin === BraveWallet.CoinType.BTC && (
                  <OrdinalsWarningMessage
                    acknowledged={isWarningAcknowledged}
                    onChange={setIsWarningAcknowledged}
                  />
                )}
                {isZCashShieldedTransactionsEnabled
                  && tokenFromParams?.coin === BraveWallet.CoinType.ZEC
                  && getZCashTransactionTypeResult
                  && toAddressOrUrl
                  && (getZCashTransactionTypeResult.txType
                    === BraveWallet.ZCashTxType.kTransparentToOrchard
                    || getZCashTransactionTypeResult.txType
                      === BraveWallet.ZCashTxType.kOrchardToOrchard) && (
                    <AddMemo
                      memoText={memoText}
                      onUpdateMemoText={setMemoText}
                    />
                  )}
                {isShieldingFunds && (
                  <Row
                    width='100%'
                    padding='16px 0px 0px 0px'
                  >
                    <ShieldingFundsAlert type='info'>
                      {getLocale('braveWalletShieldingFundsAlertDescription')}
                    </ShieldingFundsAlert>
                  </Row>
                )}
              </Column>
              <ReviewButtonRow width='100%'>
                <LeoSquaredButton
                  onClick={submitSend}
                  size='large'
                  isDisabled={
                    memoText.length > MAX_ZCASH_MEMO_LENGTH
                    || !toAddressOrUrl
                    || insufficientFundsError
                    || sendAmount === ''
                    || parseFloat(sendAmount) === 0
                    || Boolean(sendAmountValidationError)
                    || (tokenFromParams?.coin === BraveWallet.CoinType.BTC
                      && !isWarningAcknowledged)
                    || isAccountSyncing
                  }
                >
                  {getLocale(
                    getReviewButtonText(
                      sendAmountValidationError,
                      insufficientFundsError,
                      isAccountSyncing,
                      isShieldingFunds,
                    ),
                  ).replace('$1', CoinTypesMap[networkFromParams?.coin ?? 0])}
                </LeoSquaredButton>
              </ReviewButtonRow>
            </Column>
          </ToSectionWrapper>
        </Column>
      </WalletPageWrapper>
      {showSelectAddressModal && (
        <SelectAddressModal
          onClose={closeSelectAddressModal}
          selectedNetwork={networkFromParams}
          fromAccountId={accountFromParams?.accountId}
          selectedAsset={tokenFromParams}
          toAddressOrUrl={toAddressOrUrl}
          setToAddressOrUrl={onChangeToAddressOrUrl}
          setResolvedDomainAddress={setResolvedDomainAddress}
          ref={selectAddressModalRef}
        />
      )}
      {showSelectTokenModal ? (
        <SelectTokenModal
          onClose={closeSelectTokenModal}
          selectedSendOption={selectedSendOption}
          ref={selectTokenModalRef}
          onSelectAsset={selectSendAsset}
          onSelectSendOption={onSelectSendOption}
          selectedFromToken={needsAccountSelected ? tokenFromParams : undefined}
          needsAccount={needsAccountSelected}
          selectingFromOrTo='from'
          modalType='send'
        />
      ) : null}
    </>
  )
})

export default SendScreen

/**
 * ETH → Wei conversion
 */
function ethToWeiAmount(
  sendAmount: string,
  selectedSendAsset: BraveWallet.BlockchainToken,
): Amount {
  return new Amount(sendAmount).multiplyByDecimals(selectedSendAsset.decimals)
}

function getReviewButtonText(
  sendAmountValidationError: string | undefined,
  insufficientFundsError: boolean,
  isAccountSyncing?: boolean,
  isShieldingFunds?: boolean,
) {
  if (sendAmountValidationError) {
    return 'braveWalletDecimalPlacesError'
  }
  if (insufficientFundsError) {
    return 'braveWalletNotEnoughFunds'
  }
  if (isAccountSyncing) {
    return 'braveWalletAccountIsSyncing'
  }
  if (isShieldingFunds) {
    return 'braveWalletReviewShield'
  }

  return 'braveWalletReviewSend'
}
