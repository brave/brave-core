// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { create } from 'ethereum-blockies'

// Actions
import { PanelActions } from '../../../panel/actions'

// Types
import { BraveWallet, PanelState, WalletState } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { isHardwareAccount } from '../../../utils/address-utils'
import { findAccountName } from '../../../utils/account-utils'

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
  WarningTitleRow,
  WarningIcon
} from './style'

import {
  QueueStepRow,
  QueueStepButton,
  QueueStepText
} from '../confirm-transaction-panel/style'

import {
  TabRow,
  WarningBox,
  WarningTitle,
  WarningText,
  LearnMoreButton,
  URLText
} from '../shared-panel-styles'

import { DetailColumn } from '../transaction-box/style'
import { getSolanaSystemInstructionParamsAndType } from '../../../utils/solana-instruction-utils'
import { Tooltip } from '../../shared'

export interface Props {
  signMode: 'signTx' | 'signAllTxs'
}

enum SignDataSteps {
  SignRisk = 0,
  SignData = 1
}

export const SignTransactionPanel = ({ signMode }: Props) => {
  // redux
  const dispatch = useDispatch()
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)
  const signTransactionRequests = useSelector(({ panel }: { panel: PanelState }) => panel.signTransactionRequests)
  const signAllTransactionsRequests = useSelector(({ panel }: { panel: PanelState }) => panel.signAllTransactionsRequests)
  const signTransactionData = signMode === 'signTx' ? signTransactionRequests : signAllTransactionsRequests

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
  const onCancel = () => {
    if (!selectedQueueData) {
      return
    }

    if (isHardwareAccount(accounts, selectedQueueData.fromAddress)) {
      // TODO: waiting on SOL harware support
      // dispatch(PanelActions.signAllTransactionsHardwareProcessed({})
      // dispatch(PanelActions.signTransactionHardwareProcessed({
      //   success: false,
      //   id: selectedQueueData.id,
      //   signature: '',
      //   error: ''
      // }))
    } else {
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
    }
  }

  const onSign = () => {
    if (!selectedQueueData) {
      return
    }

    if (isHardwareAccount(accounts, selectedQueueData.fromAddress)) {
      // TODO: waiting on SOL harware support
      // dispatch(PanelActions.signTransactionHardware(selectedQueueData))
      // dispatch(PanelActions.signAllTransactionsHardware(selectedQueueData))
    } else {
      if (signMode === 'signTx') {
        dispatch(PanelActions.signTransactionProcessed({
          approved: true,
          id: selectedQueueData.id
        }))
        return
      }
      if (signMode === 'signAllTxs') {
        dispatch(PanelActions.signAllTransactionsProcessed({
          approved: true,
          id: selectedQueueData.id
        }))
      }
    }
  }

  const onContinueSigning = () => {
    setSignStep(SignDataSteps.SignData)
  }

  const onClickLearnMore = () => {
    window.open('https://support.brave.com/hc/en-us/articles/4409513799693', '_blank')
  }

  const onQueueNextSignTransaction = () => {
    if (signTransactionQueueInfo.queueNumber === signTransactionQueueInfo.queueLength) {
      setSelectedQueueData(selectedQueueData)
      return
    }
    setSelectedQueueData(signTransactionData[signTransactionQueueInfo.queueNumber])
  }

  // effects
  React.useEffect(() => {
    setSelectedQueueData(signTransactionData?.[0] || undefined)
  }, [signTransactionData])

  // render

  return (
    <StyledWrapper>

      <TopRow>
        <NetworkText>{'Solana'}</NetworkText>
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
                    typedInstructionWithParams={getSolanaSystemInstructionParamsAndType(instruction)}
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
          text={getLocale('braveWalletBackupButtonCancel')}
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
