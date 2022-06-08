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
import NavButton from '../buttons/nav-button'
import { PanelTab } from '../panel-tab'
import { CreateSiteOrigin } from '../../shared'

// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  AccountNameText,
  TopRow,
  NetworkText,
  PanelTitle,
  MessageBox,
  MessageText,
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

export interface Props {
  showWarning?: boolean
  signMode: 'signTx' | 'signAllTxs'
}

enum SignDataSteps {
  SignRisk = 0,
  SignData = 1
}

export const SignTransactionPanel = ({
  signMode,
  showWarning
}: Props) => {
  // redux
  const dispatch = useDispatch()
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)
  const signTransactionRequests = useSelector(({ panel }: { panel: PanelState }) => panel.signTransactionRequests)
  const signAllTransactionsRequests = useSelector(({ panel }: { panel: PanelState }) => panel.signAllTransactionsRequests)
  const signTransactionData = signMode === 'signTx' ? signTransactionRequests : signAllTransactionsRequests

  // state
  const [signStep, setSignStep] = React.useState<SignDataSteps>(showWarning ? SignDataSteps.SignRisk : SignDataSteps.SignData)
  const [selectedQueueData, setSelectedQueueData] = React.useState<BraveWallet.SignTransactionRequest | BraveWallet.SignAllTransactionsRequest | undefined>(undefined)

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

  const isDisabled = React.useMemo((): boolean => signTransactionData.findIndex(
    (data) =>
      data.id === selectedQueueData?.id) !== 0
    , [signTransactionData, selectedQueueData]
  )

  // methods
  const onCancel = () => {
    if (!selectedQueueData) {
      return
    }

    if (isHardwareAccount(accounts, selectedQueueData.fromAddress)) {
      // dispatch(PanelActions.signMessageHardwareProcessed({
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
      // dispatch(PanelActions.signMessageHardware(selectedQueueData))
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

      <AccountNameText>{findAccountName(accounts, selectedQueueData?.fromAddress || '') ?? ''}</AccountNameText>

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
              text={getLocale('braveWalletSignTransactionMessageTitle')}
            />
          </TabRow>
          <MessageBox>
            <MessageText>{
              signMode === 'signAllTxs'
                ? ((selectedQueueData as BraveWallet.SignAllTransactionsRequest)?.txDatas || []).map(d => {
                  return `${Object.keys(BraveWallet.TransactionType)[d.solanaTxData?.txType || 0]} ${d.solanaTxData?.lamports.toString()}`
                })
                : 'signTx'
            }</MessageText>
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
