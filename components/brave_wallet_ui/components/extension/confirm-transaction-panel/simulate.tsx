// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

import { WalletState, BraveWallet } from '../../../constants/types'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import Amount from '../../../utils/amount'
import { getLocale } from '$web-common/locale'

// Hooks
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import { useLib } from '../../../common/hooks'

// Components
import { PanelTab, TransactionDetailBox } from '..'
import AdvancedTransactionSettings from '../advanced-transaction-settings'

// Styled Components
import {
  StyledWrapper,
  AccountNameText,
  TopRow,
  NetworkText,
  ArrowIcon,
  FromToRow,
  EditButton
} from './style'
import {
  Box,
  BoxTitle,
  InfoCircleIcon,
  Divider,
  BoxRow,
  BoxSubTitle,
  BoxColumn,
  Loader
} from './simulate.style'
import { Skeleton } from '../../shared/loading-skeleton/styles'
import { TabRow } from '../shared-panel-styles'
import Tooltip from '../../shared/tooltip/index'

import { Footer } from './common/footer'
import { TransactionQueueStep } from './common/queue'
import { Origin } from './common/origin'
import { EditPendingTransactionGas } from './common/gas'
import {
  NetworkFeeAndSettingsContainer,
  NetworkFeeContainer,
  NetworkFeeTitle,
  NetworkFeeValue,
  Settings,
  SettingsIcon
} from './swap.style'
import { CreateNetworkIcon, LoadingSkeleton } from '../../shared'

type confirmPanelTabs = 'transaction' | 'details'

interface Props {
  onConfirm: () => void
  onReject: () => void
}

export const ConfirmSimulatedTransaction = (props: Props) => {
  const { onConfirm, onReject } = props

  const { simulateTransaction } = useLib()

  // redux
  const activeOrigin = useSelector(({ wallet }: { wallet: WalletState }) => wallet.activeOrigin)
  const defaultCurrencies = useSelector(
    ({ wallet }: { wallet: WalletState }) => wallet.defaultCurrencies
  )
  const transactionInfo = useSelector(
    ({ wallet }: { wallet: WalletState }) => wallet.selectedPendingTransaction
  )

  const originInfo = transactionInfo?.originInfo ?? activeOrigin

  // custom hooks
  const {
    fromAccountName,
    transactionDetails,
    transactionsNetwork,
    updateUnapprovedTransactionNonce
  } = usePendingTransactions()

  // state
  const [selectedTab, setSelectedTab] = React.useState<confirmPanelTabs>('transaction')
  const [isEditing, setIsEditing] = React.useState<boolean>(false)
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] =
    React.useState<boolean>(false)
  const [simulationReport, setSimulationReport] = React.useState<
    BraveWallet.SimulationReport | undefined
  >(undefined)

  // methods
  const onSelectTab = (tab: confirmPanelTabs) => () => setSelectedTab(tab)

  const onToggleEditGas = () => setIsEditing(prev => !prev)

  const onToggleAdvancedTransactionSettings = () => {
    setShowAdvancedTransactionSettings(prev => !prev)
  }

  // render
  if (!transactionDetails || !transactionInfo) {
    return (
      <StyledWrapper>
        <Skeleton width={'100%'} height={'100%'} enableAnimation />
      </StyledWrapper>
    )
  }

  React.useEffect(() => {
    ;(async () => {
      const { report, success } = await simulateTransaction(transactionInfo)
      if (success) {
        setSimulationReport(report)
      }
    })()
  }, [transactionInfo])

  if (isEditing) {
    return <EditPendingTransactionGas onCancel={onToggleEditGas} />
  }

  if (showAdvancedTransactionSettings) {
    return (
      <AdvancedTransactionSettings
        onCancel={onToggleAdvancedTransactionSettings}
        nonce={transactionDetails.nonce}
        txMetaId={transactionInfo.id}
        updateUnapprovedTransactionNonce={updateUnapprovedTransactionNonce}
      />
    )
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{transactionsNetwork?.chainName ?? ''}</NetworkText>
        <TransactionQueueStep />
      </TopRow>
      <Origin originInfo={originInfo} />
      <FromToRow>
        <Tooltip text={transactionInfo.fromAddress} isAddress={true} position='left'>
          <AccountNameText>{fromAccountName}</AccountNameText>
        </Tooltip>
        <ArrowIcon />
        <Tooltip text={transactionDetails.recipient} isAddress={true} position='right'>
          <AccountNameText>{reduceAddress(transactionDetails.recipient)}</AccountNameText>
        </Tooltip>
      </FromToRow>

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
      </TabRow>

      {selectedTab === 'transaction' ? (
        <Box isDetail={false}>
          <SimulationReport report={simulationReport} />
        </Box>
      ) : (
        <Box isDetail={true}>
          <TransactionDetailBox transactionInfo={transactionInfo} />
        </Box>
      )}

      <NetworkFeeAndSettingsContainer>
        <NetworkFeeContainer>
          <NetworkFeeTitle>Network fee</NetworkFeeTitle>
          <NetworkFeeValue>
            <CreateNetworkIcon network={transactionsNetwork} marginRight={0} />
            {transactionDetails?.gasFeeFiat ? (
              new Amount(transactionDetails.gasFeeFiat).formatAsFiat(defaultCurrencies.fiat)
            ) : (
              <LoadingSkeleton width={38} />
            )}
            <EditButton onClick={onToggleEditGas}>
              {getLocale('braveWalletAllowSpendEditButton')}
            </EditButton>
          </NetworkFeeValue>
        </NetworkFeeContainer>
        <Settings onClick={onToggleAdvancedTransactionSettings}>
          <SettingsIcon />
        </Settings>
      </NetworkFeeAndSettingsContainer>

      <Footer onConfirm={onConfirm} onReject={onReject} />
    </StyledWrapper>
  )
}

function SimulationReport (props: { report: BraveWallet.SimulationReport | undefined }) {
  const { report } = props
  return (
    <>
      <BoxRow paddingTop={8} paddingBottom={8}>
        <BoxTitle>Estimated balance changes</BoxTitle>
        <InfoCircleIcon size={13.33} />
      </BoxRow>
      <Divider />

      {report === undefined && <Loader />}

      {report !== undefined && (
        <>
          <BoxRow paddingTop={8}>
            <BoxSubTitle>Spend</BoxSubTitle>
          </BoxRow>

          {report.simulation
            .filter(each => each.diff.startsWith('-'))
            .map(each => {
              return (
                <BoxRow paddingTop={4}>
                  <BoxColumn color='text02'>{each.symbol}</BoxColumn>
                  <BoxColumn color='errorIcon'>
                    {each.diff} {each.symbol}
                  </BoxColumn>
                </BoxRow>
              )
            })}

          <BoxRow paddingTop={8}>
            <BoxSubTitle>Receive</BoxSubTitle>
          </BoxRow>

          {report.simulation
            .filter(each => !each.diff.startsWith('-'))
            .map(each => {
              return (
                <BoxRow paddingTop={4}>
                  <BoxColumn color='text02'>{each.symbol}</BoxColumn>
                  <BoxColumn color='successIcon'>
                    +{each.diff} {each.symbol}
                  </BoxColumn>
                </BoxRow>
              )
            })}
        </>
      )}
    </>
  )
}

export default ConfirmSimulatedTransaction
