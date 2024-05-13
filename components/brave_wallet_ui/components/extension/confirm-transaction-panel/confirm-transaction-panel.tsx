// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import { WalletSelectors } from '../../../common/selectors'

// Hooks
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import { useExplorer } from '../../../common/hooks/explorer'
import {
  useGetAddressByteCodeQuery,
  useGetDefaultFiatCurrencyQuery
} from '../../../common/slices/api.slice'
import {
  useUnsafeWalletSelector //
} from '../../../common/hooks/use-safe-selector'

// Components
import CreateSiteOrigin from '../../shared/create-site-origin/index'
import Tooltip from '../../shared/tooltip/index'
import withPlaceholderIcon from '../../shared/create-placeholder-icon'
import { PanelTab } from '../panel-tab/index'
import { TransactionDetailBox } from '../transaction-box/index'
import EditAllowance from '../edit-allowance'
import AdvancedTransactionSettingsButton from '../advanced-transaction-settings/button'
import AdvancedTransactionSettings from '../advanced-transaction-settings'
import { TransactionInfo } from './transaction-info'
import { NftIcon } from '../../shared/nft-icon/nft-icon'
import {
  PendingTransactionActionsFooter //
} from './common/pending_tx_actions_footer'
import { TransactionQueueSteps } from './common/queue'
import { Origin } from './common/origin'
import { EditPendingTransactionGas } from './common/gas'

// Styled Components
import {
  StyledWrapper,
  FromCircle,
  ToCircle,
  AccountNameText,
  TopRow,
  NetworkText,
  TransactionAmountBig,
  TransactionFiatAmountBig,
  MessageBox,
  TransactionTypeText,
  AccountCircleWrapper,
  ArrowIcon,
  EditButton,
  WarningIcon,
  ContractButton,
  ExplorerIcon
} from './style'
import { Skeleton } from '../../shared/loading-skeleton/styles'

import {
  TabRow,
  Description,
  PanelTitle,
  AddressAndOrb,
  AddressText,
  WarningBox,
  WarningTitle,
  LearnMoreButton,
  WarningBoxTitleRow,
  URLText
} from '../shared-panel-styles'
import { Column, Row } from '../../shared/style'

type confirmPanelTabs = 'transaction' | 'details'

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 0 } as const
const NftAssetIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_CONFIG)

const onClickLearnMore = () => {
  chrome.tabs.create(
    { url: 'https://support.brave.com/hc/en-us/articles/5546517853325' },
    () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    }
  )
}

export const ConfirmTransactionPanel = () => {
  // redux
  const activeOrigin = useUnsafeWalletSelector(WalletSelectors.activeOrigin)

  // queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()

  // custom hooks
  const {
    erc20ApproveTokenInfo,
    fromAccount,
    fromOrb,
    isERC20Approve,
    isERC721SafeTransferFrom,
    isERC721TransferFrom,
    isEthereumTransaction,
    isAssociatedTokenAccountCreation,
    onEditAllowanceSave,
    toOrb,
    transactionDetails,
    transactionsNetwork,
    transactionTitle,
    updateUnapprovedTransactionNonce,
    isCurrentAllowanceUnlimited,
    currentTokenAllowance,
    selectedPendingTransaction,
    onConfirm,
    onReject,
    gasFee,
    insufficientFundsError,
    insufficientFundsForGasError,
    queueNextTransaction,
    transactionQueueNumber,
    transactionsQueueLength,
    isSolanaTransaction,
    isBitcoinTransaction,
    isZCashTransaction,
    hasFeeEstimatesError,
    isLoadingGasFee,
    rejectAllTransactions,
    isConfirmButtonDisabled
  } = usePendingTransactions()

  // queries
  const { data: byteCode, isLoading } = useGetAddressByteCodeQuery(
    transactionDetails && isEthereumTransaction
      ? {
          address: transactionDetails?.recipient ?? '',
          coin: transactionDetails?.coinType ?? -1,
          chainId: transactionDetails?.chainId ?? ''
        }
      : skipToken
  )

  // computed
  const isContract =
    !isLoading && isEthereumTransaction && byteCode && byteCode !== '0x'
  const originInfo = selectedPendingTransaction?.originInfo ?? activeOrigin

  // hooks
  const onClickViewOnBlockExplorer = useExplorer(transactionsNetwork)

  // state
  const [selectedTab, setSelectedTab] =
    React.useState<confirmPanelTabs>('transaction')
  const [isEditing, setIsEditing] = React.useState<boolean>(false)
  const [isEditingAllowance, setIsEditingAllowance] =
    React.useState<boolean>(false)
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] =
    React.useState<boolean>(false)
  const [isWarningCollapsed, setIsWarningCollapsed] = React.useState(true)

  // methods
  const onSelectTab = (tab: confirmPanelTabs) => () => setSelectedTab(tab)

  const onToggleEditGas = () => setIsEditing((prev) => !prev)

  const onToggleEditAllowance = () => setIsEditingAllowance((prev) => !prev)

  const onToggleAdvancedTransactionSettings = () => {
    setShowAdvancedTransactionSettings((prev) => !prev)
  }

  // render
  if (!transactionDetails || !selectedPendingTransaction || !fromAccount) {
    return (
      <StyledWrapper>
        <Skeleton
          width={'100%'}
          height={'100%'}
          enableAnimation
        />
      </StyledWrapper>
    )
  }

  if (isEditing) {
    return <EditPendingTransactionGas onCancel={onToggleEditGas} />
  }

  if (isEditingAllowance) {
    return (
      <EditAllowance
        onCancel={onToggleEditAllowance}
        onSave={onEditAllowanceSave}
        proposedAllowance={transactionDetails.valueExact}
        symbol={transactionDetails.symbol}
        approvalTarget={transactionDetails.approvalTargetLabel || ''}
        isApprovalUnlimited={transactionDetails.isApprovalUnlimited || false}
      />
    )
  }

  if (showAdvancedTransactionSettings) {
    return (
      <AdvancedTransactionSettings
        onCancel={onToggleAdvancedTransactionSettings}
        nonce={transactionDetails.nonce}
        chainId={selectedPendingTransaction.chainId}
        txMetaId={selectedPendingTransaction.id}
        updateUnapprovedTransactionNonce={updateUnapprovedTransactionNonce}
      />
    )
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{transactionsNetwork?.chainName ?? ''}</NetworkText>

        <TransactionQueueSteps
          queueNextTransaction={queueNextTransaction}
          transactionQueueNumber={transactionQueueNumber}
          transactionsQueueLength={transactionsQueueLength}
        />
      </TopRow>

      {isERC20Approve ? (
        <>
          <Origin originInfo={originInfo} />
          <PanelTitle>
            {getLocale('braveWalletAllowSpendTitle').replace(
              '$1',
              erc20ApproveTokenInfo?.symbol ?? ''
            )}
          </PanelTitle>
          <AddressAndOrb>
            <Tooltip
              text={transactionDetails.approvalTarget}
              isAddress={true}
              position={'right'}
            >
              <AddressText>
                {transactionDetails.approvalTargetLabel}
              </AddressText>
            </Tooltip>
          </AddressAndOrb>
          <Description>
            {getLocale('braveWalletAllowSpendDescription').replace(
              '$1',
              erc20ApproveTokenInfo?.symbol ?? ''
            )}
          </Description>

          {transactionDetails.isApprovalUnlimited && (
            <WarningBox warningType={'danger'}>
              <WarningBoxTitleRow>
                <WarningIcon />
                <WarningTitle warningType={'danger'}>
                  {getLocale('braveWalletAllowSpendUnlimitedWarningTitle')}
                </WarningTitle>
              </WarningBoxTitleRow>
            </WarningBox>
          )}

          <EditButton onClick={onToggleEditAllowance}>
            {getLocale('braveWalletEditPermissionsButton')}
          </EditButton>
        </>
      ) : (
        <>
          <AccountCircleWrapper>
            <FromCircle orb={fromOrb} />
            <ToCircle orb={toOrb} />
          </AccountCircleWrapper>
          <URLText>
            <CreateSiteOrigin
              originSpec={originInfo.originSpec}
              eTldPlusOne={originInfo.eTldPlusOne}
            />
          </URLText>

          <Row
            marginBottom={8}
            maxWidth={isContract ? '90%' : 'unset'}
            width={'100%'}
            gap={'8px'}
            $wrap
          >
            <Tooltip
              text={fromAccount.address}
              isVisible={!!fromAccount.address}
              isAddress={true}
              position={'left'}
            >
              <AccountNameText>{fromAccount.name}</AccountNameText>
            </Tooltip>

            {transactionDetails.recipient &&
              transactionDetails.recipient !== fromAccount.address && (
                <>
                  <ArrowIcon />
                  {isContract ? (
                    <Column
                      alignItems={'flex-start'}
                      justifyContent={'flex-start'}
                    >
                      <NetworkText>
                        {getLocale('braveWalletNFTDetailContractAddress')}
                      </NetworkText>
                      <ContractButton
                        onClick={onClickViewOnBlockExplorer(
                          'contract',
                          `${transactionDetails.recipient}`
                        )}
                      >
                        {reduceAddress(transactionDetails.recipient)}{' '}
                        <ExplorerIcon />
                      </ContractButton>
                    </Column>
                  ) : (
                    <Tooltip
                      text={transactionDetails.recipient}
                      isAddress={true}
                      position='right'
                    >
                      <AccountNameText>
                        {reduceAddress(transactionDetails.recipient)}
                      </AccountNameText>
                    </Tooltip>
                  )}
                </>
              )}
          </Row>

          <TransactionTypeText>{transactionTitle}</TransactionTypeText>

          {(isERC721TransferFrom || isERC721SafeTransferFrom) && (
            <NftAssetIconWithPlaceholder
              asset={transactionDetails.erc721BlockchainToken}
            />
          )}

          <TransactionAmountBig>
            {isERC721TransferFrom || isERC721SafeTransferFrom
              ? transactionDetails.erc721BlockchainToken?.name +
                ' ' +
                transactionDetails.erc721TokenId
              : new Amount(transactionDetails.valueExact).formatAsAsset(
                  undefined,
                  transactionDetails.symbol
                )}
          </TransactionAmountBig>

          {!isERC721TransferFrom && !isERC721SafeTransferFrom && (
            <TransactionFiatAmountBig>
              {new Amount(transactionDetails.fiatValue).formatAsFiat(
                defaultFiatCurrency
              )}
            </TransactionFiatAmountBig>
          )}

          {isAssociatedTokenAccountCreation && (
            <WarningBox warningType={'warning'}>
              <WarningBoxTitleRow>
                <WarningTitle warningType={'warning'}>
                  {getLocale('braveWalletConfirmTransactionAccountCreationFee')}
                  <LearnMoreButton onClick={onClickLearnMore}>
                    {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
                  </LearnMoreButton>
                </WarningTitle>
              </WarningBoxTitleRow>
            </WarningBox>
          )}
        </>
      )}

      <TabRow>
        <PanelTab
          isSelected={selectedTab === 'transaction'}
          onSubmit={onSelectTab('transaction')}
          text='Transaction'
        />
        <PanelTab
          isSelected={selectedTab === 'details'}
          onSubmit={onSelectTab('details')}
          text='Details'
        />
        {isEthereumTransaction && (
          <AdvancedTransactionSettingsButton
            onSubmit={onToggleAdvancedTransactionSettings}
          />
        )}
      </TabRow>

      {isWarningCollapsed && (
        <MessageBox isDetails={selectedTab === 'details'}>
          {selectedTab === 'transaction' ? (
            <TransactionInfo
              onToggleEditGas={
                isSolanaTransaction || isBitcoinTransaction
                  ? undefined
                  : onToggleEditGas
              }
              isZCashTransaction={isZCashTransaction}
              isBitcoinTransaction={isBitcoinTransaction}
              transactionDetails={transactionDetails}
              isERC721SafeTransferFrom={isERC721SafeTransferFrom}
              isERC721TransferFrom={isERC721TransferFrom}
              transactionsNetwork={transactionsNetwork}
              hasFeeEstimatesError={Boolean(hasFeeEstimatesError)}
              isLoadingGasFee={isLoadingGasFee}
              gasFee={gasFee}
              insufficientFundsError={insufficientFundsError}
              insufficientFundsForGasError={insufficientFundsForGasError}
              isERC20Approve={isERC20Approve}
              currentTokenAllowance={currentTokenAllowance}
              isCurrentAllowanceUnlimited={isCurrentAllowanceUnlimited}
            />
          ) : (
            <TransactionDetailBox
              transactionInfo={selectedPendingTransaction}
              instructions={transactionDetails.instructions}
            />
          )}
        </MessageBox>
      )}

      <PendingTransactionActionsFooter
        onConfirm={onConfirm}
        onReject={onReject}
        rejectAllTransactions={rejectAllTransactions}
        isConfirmButtonDisabled={isConfirmButtonDisabled}
        transactionDetails={transactionDetails}
        transactionsQueueLength={transactionsQueueLength}
        insufficientFundsForGasError={insufficientFundsForGasError}
        insufficientFundsError={insufficientFundsError}
        isWarningCollapsed={isWarningCollapsed}
        setIsWarningCollapsed={setIsWarningCollapsed}
      />
    </StyledWrapper>
  )
}

export default ConfirmTransactionPanel
