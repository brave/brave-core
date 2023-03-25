// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { create } from 'ethereum-blockies'

// Actions
import { PanelActions } from '../../../panel/actions'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { isHardwareAccount } from '../../../utils/address-utils'
import { findAccountName } from '../../../utils/account-utils'
import { useUnsafePanelSelector, useUnsafeWalletSelector } from '../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../common/selectors'
import { PanelSelectors } from '../../../panel/selectors'
import { useGetDefaultNetworksQuery } from '../../../common/slices/api.slice'

// Components
import NavButton from '../buttons/nav-button/index'
import PanelTab from '../panel-tab/index'
import CreateSiteOrigin from '../../shared/create-site-origin/index'
import SolanaTransactionInstruction from '../../shared/solana-transaction-instruction/solana-transaction-instruction'

// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  AccountNameText,
  TopRow,
  NetworkText,
  PanelTitle,
  MessageBox,
  ButtonRow,
  WarningTitleRow
} from './style'

import {
  QueueStepRow,
  QueueStepButton,
  QueueStepText
} from '../confirm-transaction-panel/common/style'

import {
  TabRow,
  WarningBox,
  WarningTitle,
  WarningText,
  LearnMoreButton,
  URLText,
  WarningIcon
} from '../shared-panel-styles'

import { DetailColumn } from '../transaction-box/style'
import { getSolanaTransactionInstructionParamsAndType } from '../../../utils/solana-instruction-utils'
import { Tooltip } from '../../shared'

export interface Props {
  signMode: 'signTx' | 'signAllTxs'
}

enum SignDataSteps {
  SignRisk = 0,
  SignData = 1
}

const onClickLearnMore = () => {
  window.open('https://support.brave.com/hc/en-us/articles/4409513799693', '_blank', 'noreferrer')
}

export const SignTransactionPanel = ({ signMode }: Props) => {
  // redux
  const dispatch = useDispatch()
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const signTransactionRequests = useUnsafePanelSelector(
    PanelSelectors.signTransactionRequests
  )
  const signAllTransactionsRequests = useUnsafePanelSelector(
    PanelSelectors.signAllTransactionsRequests
  )

  // queries
  const { data: defaultNetworks } = useGetDefaultNetworksQuery()

  const signTransactionData =
    signMode === 'signTx'
      ? signTransactionRequests
      : signAllTransactionsRequests

  const network = defaultNetworks?.find(
    (net) => net.coin === signTransactionData[0].coin
  )

  // state
  const [signStep, setSignStep] = React.useState<SignDataSteps>(SignDataSteps.SignRisk)
  const [selectedQueueData, setSelectedQueueData] = React.useState<
    BraveWallet.SignTransactionRequest | BraveWallet.SignAllTransactionsRequest | undefined
  >(undefined)

  // memos
  const orb = React.useMemo(() => {
    return create({ seed: selectedQueueData?.fromAddress?.toLowerCase() ?? '', size: 8, scale: 16 }).toDataURL()
  }, [selectedQueueData?.fromAddress])

  const signTransactionQueueInfo = React.useMemo(() => {
    return {
      queueLength: signTransactionData.length,
      queueNumber: signTransactionData.findIndex((data) => data.id === selectedQueueData?.id) + 1
    }
  }, [signTransactionData, selectedQueueData])

  const isDisabled = React.useMemo((): boolean =>
    signTransactionData.findIndex((data) => data.id === selectedQueueData?.id) !== 0,
    [signTransactionData, selectedQueueData]
  )

  const txDatas = React.useMemo(() => {
    return (
      (selectedQueueData as BraveWallet.SignAllTransactionsRequest)?.txDatas
        ? (selectedQueueData as BraveWallet.SignAllTransactionsRequest)?.txDatas.map(({ solanaTxData }) => solanaTxData)
        : [(selectedQueueData as BraveWallet.SignTransactionRequest)?.txData?.solanaTxData]
    ).filter((data): data is BraveWallet.SolanaTxData => !!data)
  }, [selectedQueueData])

  // methods
  const onCancel = React.useCallback(() => {
    if (!selectedQueueData) {
      return
    }

    if (signMode === 'signTx') {
      dispatch(PanelActions.signTransactionProcessed({
        approved: false,
        id: selectedQueueData.id
      }))
      return
    }
    if (signMode === 'signAllTxs') {
      dispatch(PanelActions.signAllTransactionsProcessed({
        approved: false,
        id: selectedQueueData.id
      }))
    }
  }, [selectedQueueData, accounts, signMode])

  const onSign = React.useCallback(() => {
    if (!selectedQueueData) {
      return
    }

    const isHwAccount = isHardwareAccount(accounts, selectedQueueData.fromAddress)
    if (signMode === 'signTx') {
      if (isHwAccount) {
        dispatch(PanelActions.signTransactionHardware(selectedQueueData as BraveWallet.SignTransactionRequest))
        return
      }
      dispatch(PanelActions.signTransactionProcessed({
        approved: true,
        id: selectedQueueData.id
      }))
      return
    }
    if (signMode === 'signAllTxs') {
      if (isHwAccount) {
        dispatch(PanelActions.signAllTransactionsHardware(selectedQueueData as BraveWallet.SignAllTransactionsRequest))
        return
      }
      dispatch(PanelActions.signAllTransactionsProcessed({
        approved: true,
        id: selectedQueueData.id
      }))
    }
  }, [selectedQueueData, accounts, signMode])

  const onContinueSigning = React.useCallback(() => {
    setSignStep(SignDataSteps.SignData)
  }, [])

  const onQueueNextSignTransaction = React.useCallback(() => {
    if (signTransactionQueueInfo.queueNumber === signTransactionQueueInfo.queueLength) {
      setSelectedQueueData(signTransactionData[0])
      return
    }
    setSelectedQueueData(signTransactionData[signTransactionQueueInfo.queueNumber])
  }, [signTransactionQueueInfo, signTransactionData])

  // effects
  React.useEffect(() => {
    setSelectedQueueData(signTransactionData?.[0] || undefined)
  }, [signTransactionData])

  // render
  return (
    <StyledWrapper>

      <TopRow>
        <NetworkText>{network?.chainName ?? ''}</NetworkText>
        {signTransactionQueueInfo.queueLength > 1 &&
          <QueueStepRow>
            <QueueStepText>{signTransactionQueueInfo.queueNumber} {getLocale('braveWalletQueueOf')} {signTransactionQueueInfo.queueLength}</QueueStepText>
            <QueueStepButton
              onClick={onQueueNextSignTransaction}
            >
              {signTransactionQueueInfo.queueNumber === signTransactionQueueInfo.queueLength
                ? getLocale('braveWalletQueueFirst')
                : getLocale('braveWalletQueueNext')
              }
            </QueueStepButton>
          </QueueStepRow>
        }
      </TopRow>

      <AccountCircle orb={orb} />

      {selectedQueueData &&
        <URLText>
          <CreateSiteOrigin
            originSpec={selectedQueueData.originInfo.originSpec}
            eTldPlusOne={selectedQueueData.originInfo.eTldPlusOne}
          />
        </URLText>
      }

      <Tooltip
        text={selectedQueueData?.fromAddress || ''}
        isAddress
      >
        <AccountNameText>
          {findAccountName(accounts, selectedQueueData?.fromAddress || '') ?? ''}
        </AccountNameText>
      </Tooltip>

      <PanelTitle>{getLocale('braveWalletSignTransactionTitle')}</PanelTitle>

      {signStep === SignDataSteps.SignRisk &&
        <WarningBox warningType='danger'>
          <WarningTitleRow>
            <WarningIcon />
            <WarningTitle warningType='danger'>{getLocale('braveWalletSignWarningTitle')}</WarningTitle>
          </WarningTitleRow>
          <WarningText>{getLocale('braveWalletSignWarning')}</WarningText>
          <LearnMoreButton onClick={onClickLearnMore}>{getLocale('braveWalletAllowAddNetworkLearnMoreButton')}</LearnMoreButton>
        </WarningBox>
      }

      {signStep === SignDataSteps.SignData &&
        <>
          <TabRow>
            <PanelTab
              isSelected={true}
              text={getLocale('braveWalletAccountSettingsDetails')}
            />
          </TabRow>
          <MessageBox>
            {txDatas.map(({ instructions, txType }, i) => {
              return <DetailColumn key={`${txType}-${i}`}>
                {instructions?.map((instruction, index) => {
                  return <SolanaTransactionInstruction
                    key={index}
                    typedInstructionWithParams={getSolanaTransactionInstructionParamsAndType(instruction)}
                  />
                })}
              </DetailColumn>
            })}
          </MessageBox>
        </>
      }
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletButtonCancel')}
          onSubmit={onCancel}
          disabled={isDisabled}
        />
        <NavButton
          buttonType={signStep === SignDataSteps.SignData ? 'sign' : 'danger'}
          text={signStep === SignDataSteps.SignData ? getLocale('braveWalletSignTransactionButton') : getLocale('braveWalletButtonContinue')}
          onSubmit={signStep === SignDataSteps.SignRisk ? onContinueSigning : onSign}
          disabled={isDisabled}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default SignTransactionPanel
