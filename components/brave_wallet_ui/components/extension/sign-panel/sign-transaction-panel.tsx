// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet, SignDataSteps } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import {
  getSolanaTransactionInstructionParamsAndType as getTypedSolTxInstruction //
} from '../../../utils/solana-instruction-utils'
import { getTxDatasFromQueuedSolSignRequest } from '../../../utils/tx-utils'

// Hooks
import { useAccountOrb } from '../../../common/hooks/use-orb'
import {
  useProcessSignSolanaTransaction //
} from '../../../common/hooks/use_sign_solana_tx_queue'

// Components
import NavButton from '../buttons/nav-button/index'
import PanelTab from '../panel-tab/index'
import CreateSiteOrigin from '../../shared/create-site-origin/index'
import SolanaTransactionInstruction from '../../shared/solana-transaction-instruction/solana-transaction-instruction'
import {
  TxWarningBanner //
} from '../confirm-transaction-panel/common/tx_warnings'
import {
  TransactionSimulationNotSupportedSheet //
} from '../transaction_simulation_not_supported_sheet/transaction_simulation_not_supported_sheet'

// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  AccountNameText,
  TopRow,
  NetworkText,
  PanelTitle,
  MessageBox,
  SignPanelButtonRow,
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
import { Tooltip } from '../../shared/tooltip/index'
import { Column } from '../../shared/style'

interface Props {
  signMode: 'signTx' | 'signAllTxs'
  isSigningDisabled: boolean
  network: BraveWallet.NetworkInfo
  queueNextSignTransaction: () => void
  selectedQueueData:
    | BraveWallet.SignTransactionRequest
    | BraveWallet.SignAllTransactionsRequest
    | undefined
  signingAccount?: BraveWallet.AccountInfo
  queueLength: number
  queueNumber: number
  retrySimulation?: () => void
  showSimulationNotSupportedMessage?: boolean
}

// TODO: broken article link
// https://github.com/brave/brave-browser/issues/39708
const onClickLearnMore = () => {
  window.open(
    'https://support.brave.com/hc/en-us/articles/4409513799693',
    '_blank',
    'noreferrer'
  )
}

export const SignTransactionPanel = ({
  signMode,
  isSigningDisabled,
  network,
  queueLength,
  queueNextSignTransaction,
  queueNumber,
  selectedQueueData,
  signingAccount,
  retrySimulation,
  showSimulationNotSupportedMessage
}: Props) => {
  // custom hooks
  const orb = useAccountOrb(signingAccount)

  // state
  const [signStep, setSignStep] = React.useState<SignDataSteps>(
    SignDataSteps.SignRisk
  )
  const [isSimulationWarningDismissed, setIsSimulationWarningDismissed] =
    React.useState(false)

  // methods
  const onAcceptSigningRisks = React.useCallback(() => {
    setSignStep(SignDataSteps.SignData)
  }, [])

  const { cancelSign: onCancelSign, sign: onSign } =
    useProcessSignSolanaTransaction({
      signMode,
      account: signingAccount,
      request: selectedQueueData
    })

  const txDatas = React.useMemo(() => {
    return selectedQueueData
      ? getTxDatasFromQueuedSolSignRequest(selectedQueueData)
      : []
  }, [selectedQueueData])

  // render
  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{network?.chainName ?? ''}</NetworkText>
        {queueLength > 1 && (
          <QueueStepRow>
            <QueueStepText>
              {queueNumber} {getLocale('braveWalletQueueOf')} {queueLength}
            </QueueStepText>
            <QueueStepButton onClick={queueNextSignTransaction}>
              {queueNumber === queueLength
                ? getLocale('braveWalletQueueFirst')
                : getLocale('braveWalletQueueNext')}
            </QueueStepButton>
          </QueueStepRow>
        )}
      </TopRow>
      <AccountCircle orb={orb} />
      {selectedQueueData && (
        <URLText>
          <CreateSiteOrigin
            originSpec={selectedQueueData.originInfo.originSpec}
            eTldPlusOne={selectedQueueData.originInfo.eTldPlusOne}
          />
        </URLText>
      )}
      <Tooltip
        text={selectedQueueData?.fromAddress || ''}
        isAddress
      >
        <AccountNameText>{signingAccount?.name ?? ''}</AccountNameText>
      </Tooltip>
      <PanelTitle>{getLocale('braveWalletSignTransactionTitle')}</PanelTitle>
      {signStep === SignDataSteps.SignRisk && (
        <WarningBox warningType='danger'>
          <WarningTitleRow>
            <WarningIcon />
            <WarningTitle warningType='danger'>
              {getLocale('braveWalletSignWarningTitle')}
            </WarningTitle>
          </WarningTitleRow>
          <WarningText>{getLocale('braveWalletSignWarning')}</WarningText>
          <LearnMoreButton onClick={onClickLearnMore}>
            {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
          </LearnMoreButton>
        </WarningBox>
      )}
      {signStep === SignDataSteps.SignData && (
        <>
          <TabRow>
            <PanelTab
              isSelected={true}
              text={getLocale('braveWalletDetails')}
            />
          </TabRow>
          <MessageBox>
            {txDatas.map(({ instructions, txType }, i) => {
              return (
                <DetailColumn key={`${txType}-${i}`}>
                  {instructions?.map((instruction, index) => {
                    return (
                      <SolanaTransactionInstruction
                        key={index}
                        typedInstructionWithParams={getTypedSolTxInstruction(
                          instruction
                        )}
                      />
                    )
                  })}
                </DetailColumn>
              )
            })}
          </MessageBox>
        </>
      )}
      <Column
        fullWidth
        gap={'8px'}
      >
        {retrySimulation && !isSimulationWarningDismissed && (
          <TxWarningBanner
            onDismiss={() => setIsSimulationWarningDismissed(true)}
            retrySimulation={retrySimulation}
          />
        )}
        <SignPanelButtonRow>
          <NavButton
            buttonType='secondary'
            text={getLocale('braveWalletButtonCancel')}
            onSubmit={onCancelSign}
            disabled={isSigningDisabled}
          />
          <NavButton
            buttonType={signStep === SignDataSteps.SignData ? 'sign' : 'danger'}
            text={
              signStep === SignDataSteps.SignData
                ? getLocale('braveWalletSignTransactionButton')
                : getLocale('braveWalletButtonContinue')
            }
            onSubmit={
              signStep === SignDataSteps.SignRisk
                ? onAcceptSigningRisks
                : onSign
            }
            disabled={isSigningDisabled}
          />
        </SignPanelButtonRow>
      </Column>
      {showSimulationNotSupportedMessage && (
        <TransactionSimulationNotSupportedSheet />
      )}
    </StyledWrapper>
  )
}

export default SignTransactionPanel
