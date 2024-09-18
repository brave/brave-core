// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet, SignDataSteps } from '../../../constants/types'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import {
  isUrlWarning,
  translateSimulationResultError
} from '../../../utils/tx-simulation-utils'
import {
  getSolanaTransactionInstructionParamsAndType as getTypedSolTxInstruction //
} from '../../../utils/solana-instruction-utils'
import { getLocale } from '../../../../common/locale'

// Hooks
import { useProcessSignSolanaTransaction } from '../../../common/hooks/use_sign_solana_tx_queue'

// Components
import { TransactionOrigin } from './common/origin'
import { TransactionQueueSteps } from './common/queue'
import { TransactionSimulationInfo } from './transaction_simulation_info'
import { PanelTab } from '../panel-tab/index'
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'
import NavButton from '../buttons/nav-button'
import {
  SolanaTransactionInstruction //
} from '../../shared/solana-transaction-instruction/solana-transaction-instruction'
import { CriticalWarningPopup } from './critical-warning-popup'
import {
  PendingTransactionActionsFooter //
} from './common/pending_tx_actions_footer'

// Styled Components
import { Column, ErrorText, Row } from '../../shared/style'
import {
  LearnMoreButton,
  TabRow,
  WarningBox,
  WarningIcon,
  WarningText,
  WarningTitle
} from '../shared-panel-styles'
import {
  SignPanelButtonRow,
  TopRow,
  WarningTitleRow
} from '../sign-panel/style'
import {
  AccountNameAndAddress,
  NetworkNameText
} from './confirm_simulated_tx_panel.styles'
import { MessageBox } from './style'
import { DetailColumn } from '../transaction-box/style'

type confirmPanelTabs = 'transaction' | 'details'

type Props = {
  txSimulation: BraveWallet.SolanaSimulationResponse
  isSigningDisabled: boolean
  network: BraveWallet.NetworkInfo
  queueNextSignTransaction: () => void
  signSolTransactionsRequest: BraveWallet.SignSolTransactionsRequest
  signingAccount: BraveWallet.AccountInfo
  queueLength: number
  queueNumber: number
}

// TODO: fix broken article link:
// https://github.com/brave/brave-browser/issues/39708
const onClickLearnMore = () => {
  window.open(
    'https://support.brave.com/hc/en-us/articles/4409513799693',
    '_blank',
    'noopener,noreferrer'
  )
}

/**
 * For Solana transaction signature requests
 */
export const SignSimulatedTransactionPanel = ({
  network,
  queueLength,
  queueNextSignTransaction,
  queueNumber,
  signSolTransactionsRequest,
  signingAccount,
  txSimulation,
  isSigningDisabled
}: Props) => {
  // custom hooks
  const { cancelSign: onCancelSign, sign: onSign } =
    useProcessSignSolanaTransaction({
      signSolTransactionsRequest
    })

  // state
  const [signStep, setSignStep] = React.useState<SignDataSteps>(
    SignDataSteps.SignRisk
  )
  const [isCriticalWarningPopupOpen, setIsCriticalWarningPopupOpen] =
    React.useState(
      txSimulation.action === BraveWallet.BlowfishSuggestedAction.kBlock
    )
  const [selectedTab, setSelectedTab] =
    React.useState<confirmPanelTabs>('transaction')
  const [isWarningCollapsed, setIsWarningCollapsed] = React.useState(true)

  // computed
  const simulationResultsErrorText = translateSimulationResultError(
    txSimulation?.error,
    BraveWallet.CoinType.SOL
  )

  // methods
  const onSelectTab = React.useCallback(
    (tab: confirmPanelTabs) => () => setSelectedTab(tab),
    []
  )

  // Critical Warning pop-up
  if (isCriticalWarningPopupOpen) {
    return (
      <CriticalWarningPopup
        onCancel={onCancelSign}
        onProceed={() => setIsCriticalWarningPopupOpen(false)}
      />
    )
  }

  return (
    <Column
      width={'100%'}
      height={'100%'}
      alignItems='center'
      justifyContent='space-between'
    >
      <TopRow>
        <Column
          alignItems='flex-start'
          gap={'4px'}
        >
          <NetworkNameText>{network.chainName}</NetworkNameText>
          <AccountNameAndAddress>
            <CopyTooltip
              isAddress
              text={signingAccount.address}
              tooltipText={signingAccount.address}
            >
              {signingAccount.name}{' '}
              {reduceAddress(signingAccount.address) ?? ''}
            </CopyTooltip>
          </AccountNameAndAddress>
        </Column>
        <TransactionQueueSteps
          queueNextTransaction={queueNextSignTransaction}
          transactionQueueNumber={queueNumber}
          transactionsQueueLength={queueLength}
        />
      </TopRow>

      <>
        {signStep === SignDataSteps.SignRisk ? (
          <>
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
            <SignPanelButtonRow>
              <NavButton
                buttonType='secondary'
                minWidth='45%'
                text={getLocale('braveWalletButtonCancel')}
                onSubmit={onCancelSign}
              />
              <NavButton
                buttonType={'danger'}
                minWidth='45%'
                text={getLocale('braveWalletButtonContinue')}
                onSubmit={() => {
                  setSignStep(SignDataSteps.SignData)
                }}
              />
            </SignPanelButtonRow>
          </>
        ) : (
          <>
            <Column
              fullWidth
              flex={1}
              justifyContent={'flex-start'}
            >
              <Row
                alignItems={'center'}
                justifyContent={'center'}
                padding={'0px 24px'}
                width={'100%'}
              >
                <TransactionOrigin
                  network={network}
                  originInfo={signSolTransactionsRequest.originInfo}
                  isFlagged={txSimulation.warnings.some((w) =>
                    isUrlWarning(w.kind)
                  )}
                />
              </Row>

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

              <Column
                alignItems={'flex-start'}
                justifyContent={'center'}
                width={'100%'}
              >
                {selectedTab === 'transaction' ? (
                  simulationResultsErrorText ? (
                    <MessageBox
                      isDetails={true}
                      width='100%'
                    >
                      <ErrorText>{simulationResultsErrorText}</ErrorText>
                    </MessageBox>
                  ) : (
                    <TransactionSimulationInfo
                      key={'SolanaSimulationInfo'}
                      simulation={txSimulation}
                      simulationType={'SVM'}
                      network={network}
                    />
                  )
                ) : (
                  <MessageBox
                    isDetails={true}
                    width='100%'
                  >
                    {signSolTransactionsRequest.txDatas.map(
                      ({ instructions, txType }, i) => {
                        return (
                          <DetailColumn key={`${txType}-${i}`}>
                            {instructions?.map((instruction, index) => {
                              return (
                                <SolanaTransactionInstruction
                                  key={index}
                                  typedInstructionWithParams={
                                    //
                                    getTypedSolTxInstruction(instruction)
                                  }
                                />
                              )
                            })}
                          </DetailColumn>
                        )
                      }
                    )}
                  </MessageBox>
                )}
              </Column>
            </Column>

            <PendingTransactionActionsFooter
              onConfirm={onSign}
              onReject={onCancelSign}
              isConfirmButtonDisabled={isSigningDisabled}
              transactionDetails={undefined}
              transactionsQueueLength={0}
              blowfishWarnings={txSimulation.warnings}
              isWarningCollapsed={isWarningCollapsed}
              setIsWarningCollapsed={setIsWarningCollapsed}
            />
          </>
        )}
      </>
    </Column>
  )
}
