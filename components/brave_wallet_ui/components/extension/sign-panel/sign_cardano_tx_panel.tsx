// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet, SignDataSteps } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'

// Hooks
import { useAccountOrb } from '../../../common/hooks/use-orb'
import {
  useProcessSignCardanoTransaction, //
} from '../../../common/hooks/use_sign_cardano_tx_queue'

// Components
import NavButton from '../buttons/nav-button/index'
import PanelTab from '../panel-tab/index'
import CreateSiteOrigin from '../../shared/create-site-origin/index'
import { TransactionQueueSteps } from '../confirm-transaction-panel/common/queue'
import {
  DetailColumn,
  DetailText,
  LabelText,
} from '../pending_transaction_details/pending_transaction_details.styles'
import DividerLine from '../divider'

// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  AccountNameText,
  TopRow,
  PanelTitle,
  MessageBox,
  SignPanelButtonRow,
  WarningTitleRow,
  MessageHeaderSection,
} from './style'

import {
  TabRow,
  WarningBox,
  WarningText,
  LearnMoreButton,
  URLText,
  WarningIcon,
} from '../shared-panel-styles'

import { Tooltip } from '../../shared/tooltip/index'
import { Column, VerticalDivider, Text } from '../../shared/style'

interface Props {
  selectedRequest: BraveWallet.SignCardanoTransactionRequest
  isSigningDisabled: boolean
  network: BraveWallet.NetworkInfo
  queueNextSignTransaction: () => void
  signingAccount: BraveWallet.AccountInfo
  queueLength: number
  queueNumber: number
}

// TODO: broken article link
// https://github.com/brave/brave-browser/issues/39708
const onClickLearnMore = () => {
  window.open(
    'https://support.brave.app/hc/en-us/articles/4409513799693',
    '_blank',
    'noreferrer',
  )
}

type TabName = 'rawTransaction' | 'details'

function renderCardanoTxInputs(inputs: BraveWallet.CardanoTxInput[]) {
  if (inputs.length === 0) {
    return null
  }

  return (
    <>
      <MessageHeaderSection>
        {getLocale('braveWalletInputs')}
      </MessageHeaderSection>
      {inputs.map((input, index) => {
        return (
          <DetailColumn
            gap='4px'
            key={'input' + index}
          >
            <LabelText>{getLocale('braveWalletInput')}:</LabelText>
            <DetailText>{`${input.outpointTxid}:${input.outpointIndex}`}</DetailText>
            <LabelText>{getLocale('braveWalletValue')}:</LabelText>
            <DetailText>{`${input.value ? input.value : 'N/A'}`}</DetailText>
            {input.tokens.map((token) => {
              return (
                <DetailColumn key={token.tokenIdHex}>
                  <LabelText>{getLocale('braveWalletToken')}:</LabelText>
                  <DetailText>
                    {token.tokenIdHex}:{`${token.value}`}
                  </DetailText>
                </DetailColumn>
              )
            })}
            <LabelText>{getLocale('braveWalletAddress')}:</LabelText>
            <DetailText>{`${input.address ? input.address : 'N/A'}`}</DetailText>
            <DividerLine />
          </DetailColumn>
        )
      })}
      <VerticalDivider />
    </>
  )
}

function renderCardanoTxOutputs(outputs: BraveWallet.CardanoTxOutput[]) {
  if (outputs.length === 0) {
    return null
  }

  return (
    <>
      <MessageHeaderSection>
        {getLocale('braveWalletOutputs')}
      </MessageHeaderSection>
      {outputs.map((output, index) => {
        return (
          <DetailColumn
            gap='4px'
            key={'output-external' + index}
          >
            <LabelText>{getLocale('braveWalletAddress')}:</LabelText>
            <DetailText>{`${output.address}`}</DetailText>
            <LabelText>{getLocale('braveWalletValue')}:</LabelText>
            <DetailText>{`${output.value}`}</DetailText>
            {output.tokens.map((token) => {
              return (
                <DetailColumn key={token.tokenIdHex}>
                  <LabelText>{getLocale('braveWalletToken')}:</LabelText>
                  <DetailText>
                    {token.tokenIdHex}:{`${token.value}`}
                  </DetailText>
                </DetailColumn>
              )
            })}
          </DetailColumn>
        )
      })}
      <VerticalDivider />
    </>
  )
}

function renderCardanoTxMint(mint: BraveWallet.CardanoTxMintToken[]) {
  if (mint.length === 0) {
    return null
  }

  return (
    <>
      <MessageHeaderSection>
        {getLocale('braveWalletMint')}
      </MessageHeaderSection>
      {mint.map((token, index) => {
        return (
          <DetailColumn
            gap='4px'
            key={'mint' + index}
          >
            <LabelText>{getLocale('braveWalletToken')}:</LabelText>
            <DetailText>{token.tokenIdHex}</DetailText>
            <LabelText>{getLocale('braveWalletValue')}:</LabelText>
            <DetailText>{`${token.amount}`}</DetailText>
            <DividerLine />
          </DetailColumn>
        )
      })}
      <VerticalDivider />
    </>
  )
}

function renderCardanoTxWithdrawals(
  withdrawals: BraveWallet.CardanoTxWithdrawal[],
) {
  if (withdrawals.length === 0) {
    return null
  }

  return (
    <>
      <MessageHeaderSection>
        {getLocale('braveWalletWithdrawals')}
      </MessageHeaderSection>
      {withdrawals.map((withdrawal, index) => {
        return (
          <DetailColumn
            gap='4px'
            key={'withdrawal' + index}
          >
            <LabelText>{getLocale('braveWalletAddress')}:</LabelText>
            <DetailText>{withdrawal.rewardAccount}</DetailText>
            <LabelText>{getLocale('braveWalletValue')}:</LabelText>
            <DetailText>{`${withdrawal.coin}`}</DetailText>
            <DividerLine />
          </DetailColumn>
        )
      })}
      <VerticalDivider />
    </>
  )
}

export const SignCardanoTxPanel = ({
  selectedRequest,
  isSigningDisabled,
  network,
  queueLength,
  queueNextSignTransaction,
  queueNumber,
  signingAccount,
}: Props) => {
  // custom hooks
  const orb = useAccountOrb(signingAccount)

  // state
  const [signStep, setSignStep] = React.useState<SignDataSteps>(
    SignDataSteps.SignRisk,
  )
  const [selectedTab, setSelectedTab] = React.useState<TabName>('details')

  // methods
  const onAcceptSigningRisks = React.useCallback(() => {
    setSignStep(SignDataSteps.SignData)
  }, [])

  const onSelectTab = React.useCallback(
    (tab: TabName) => () => setSelectedTab(tab),
    [],
  )

  const { cancelSign: onCancelSign, sign: onSign } =
    useProcessSignCardanoTransaction({
      request: selectedRequest,
    })

  // render
  return (
    <StyledWrapper>
      <TopRow>
        <Text
          textColor='tertiary'
          variant='small.regular'
        >
          {' '}
          {network.chainName}{' '}
        </Text>
        <TransactionQueueSteps
          queueNextTransaction={queueNextSignTransaction}
          transactionQueueNumber={queueNumber}
          transactionsQueueLength={queueLength}
        />
      </TopRow>
      <AccountCircle orb={orb} />
      <URLText
        textColor='secondary'
        variant='xSmall.regular'
      >
        <CreateSiteOrigin
          originSpec={selectedRequest.originInfo.originSpec}
          eTldPlusOne={selectedRequest.originInfo.eTldPlusOne}
        />
      </URLText>
      <Tooltip
        text={signingAccount.address || ''}
        isAddress
      >
        <AccountNameText
          textColor='secondary'
          variant='default.semibold'
        >
          {signingAccount?.name ?? ''}
        </AccountNameText>
      </Tooltip>
      <PanelTitle
        textColor='primary'
        variant='large.semibold'
      >
        {getLocale('braveWalletSignTransactionTitle')}
      </PanelTitle>
      {signStep === SignDataSteps.SignRisk && (
        <WarningBox warningType='danger'>
          <WarningTitleRow>
            <WarningIcon />
            <Text
              textColor='error'
              variant='small.semibold'
            >
              {getLocale('braveWalletSignWarningTitle')}
            </Text>
          </WarningTitleRow>
          <WarningText
            textColor='error'
            variant='small.regular'
          >
            {getLocale('braveWalletSignWarning')}
          </WarningText>
          <LearnMoreButton onClick={onClickLearnMore}>
            {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
          </LearnMoreButton>
        </WarningBox>
      )}
      {signStep === SignDataSteps.SignData && (
        <>
          <TabRow>
            <PanelTab
              isSelected={selectedTab === 'details'}
              onSubmit={onSelectTab('details')}
              text='Details'
            />
            <PanelTab
              isSelected={selectedTab === 'rawTransaction'}
              onSubmit={onSelectTab('rawTransaction')}
              text='Transaction'
            />
          </TabRow>
          {selectedTab === 'rawTransaction' ? (
            <MessageBox width='100%'>
              <DetailText>{`${selectedRequest.rawTxData}`}</DetailText>
            </MessageBox>
          ) : (
            <MessageBox width='100%'>
              {renderCardanoTxInputs(selectedRequest.inputs)}
              {renderCardanoTxOutputs(selectedRequest.outputs)}
              {renderCardanoTxMint(selectedRequest.mint)}
              {renderCardanoTxWithdrawals(selectedRequest.withdrawals)}
            </MessageBox>
          )}
        </>
      )}
      <Column
        fullWidth
        gap={'8px'}
      >
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
    </StyledWrapper>
  )
}

export default SignCardanoTxPanel
