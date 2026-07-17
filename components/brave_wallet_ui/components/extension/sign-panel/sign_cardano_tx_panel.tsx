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
import { Column, Text, VerticalDivider } from '../../shared/style'

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

interface CardanoTxDetailsToken {
  tokenId: string
  value: string
}

interface CardanoTxDetailsUtxo {
  txHash: string
  index: string
  value: string | null
  tokens: CardanoTxDetailsToken[] | null
  address: string | null
}

interface CardanoTxDetailsOutput {
  address: string
  value: string
  tokens: CardanoTxDetailsToken[]
}

interface CardanoTxDetailsWithdrawal {
  address: string
  value: string
}

interface CardanoTxDetails {
  inputs?: CardanoTxDetailsUtxo[]
  outputs?: CardanoTxDetailsOutput[]
  mint?: CardanoTxDetailsToken[]
  withdrawals?: CardanoTxDetailsWithdrawal[]
  scriptDataHash?: string
  collateral?: CardanoTxDetailsUtxo[]
  collateralReturn?: CardanoTxDetailsOutput
  totalCollateral?: string
}

const parseCardanoTxDetails = (details: string): CardanoTxDetails | null => {
  try {
    return JSON.parse(details) as CardanoTxDetails
  } catch {
    return null
  }
}

const CardanoTxDetailsTokens = ({
  tokens,
}: {
  tokens: CardanoTxDetailsToken[] | null | undefined
}) => (
  <>
    {tokens?.map((token) => (
      <DetailColumn key={token.tokenId}>
        <LabelText>{getLocale('braveWalletToken')}:</LabelText>
        <DetailText>
          {token.tokenId}:{token.value}
        </DetailText>
      </DetailColumn>
    ))}
  </>
)

const CardanoTxDetailsUtxoItem = ({
  label,
  utxo,
}: {
  label: string
  utxo: CardanoTxDetailsUtxo
}) => (
  <DetailColumn gap='8px'>
    <DetailColumn>
      <LabelText>{label}:</LabelText>
      <DetailText>{`${utxo.txHash}:${utxo.index}`}</DetailText>
    </DetailColumn>
    <DetailColumn>
      <LabelText>{getLocale('braveWalletValue')}:</LabelText>
      <DetailText>{utxo.value ?? 'N/A'}</DetailText>
    </DetailColumn>
    <CardanoTxDetailsTokens tokens={utxo.tokens} />
    <DetailColumn>
      <LabelText>{getLocale('braveWalletAddress')}:</LabelText>
      <DetailText>{utxo.address ?? 'N/A'}</DetailText>
    </DetailColumn>
    <VerticalDivider />
  </DetailColumn>
)

const CardanoTxDetailsOutputItem = ({
  label,
  output,
}: {
  label: string
  output: CardanoTxDetailsOutput
}) => (
  <DetailColumn gap='8px'>
    <DetailColumn>
      <LabelText>{label}:</LabelText>
    </DetailColumn>
    <DetailColumn>
      <LabelText>{getLocale('braveWalletValue')}:</LabelText>
      <DetailText>{output.value}</DetailText>
    </DetailColumn>
    <CardanoTxDetailsTokens tokens={output.tokens} />
    <DetailColumn>
      <LabelText>{getLocale('braveWalletAddress')}:</LabelText>
      <DetailText>{output.address}</DetailText>
    </DetailColumn>
    <VerticalDivider />
  </DetailColumn>
)

const CardanoTxDetailInputs = ({
  inputs,
}: {
  inputs: CardanoTxDetailsUtxo[] | undefined
}) => (
  <>
    {inputs?.map((input, index) => (
      <CardanoTxDetailsUtxoItem
        key={'input' + index}
        label={getLocale('braveWalletInput')}
        utxo={input}
      />
    ))}
  </>
)

const CardanoTxDetailOutputs = ({
  outputs,
}: {
  outputs: CardanoTxDetailsOutput[] | undefined
}) => (
  <>
    {outputs?.map((output, index) => (
      <CardanoTxDetailsOutputItem
        key={'output' + index}
        label={getLocale('braveWalletOutput')}
        output={output}
      />
    ))}
  </>
)

const CardanoTxDetailMint = ({
  mint,
}: {
  mint: CardanoTxDetailsToken[] | undefined
}) => {
  if (!mint || mint.length === 0) {
    return null
  }
  return (
    <DetailColumn gap='8px'>
      <LabelText>{getLocale('braveWalletMint')}:</LabelText>
      <CardanoTxDetailsTokens tokens={mint} />
      <VerticalDivider />
    </DetailColumn>
  )
}

const CardanoTxDetailWithdrawals = ({
  withdrawals,
}: {
  withdrawals: CardanoTxDetailsWithdrawal[] | undefined
}) => (
  <>
    {withdrawals?.map((withdrawal, index) => (
      <DetailColumn
        gap='8px'
        key={'withdrawal' + index}
      >
        <LabelText>{getLocale('braveWalletWithdrawals')}:</LabelText>
        <DetailColumn>
          <LabelText>{getLocale('braveWalletAddress')}:</LabelText>
          <DetailText>{withdrawal.address}</DetailText>
        </DetailColumn>
        <DetailColumn>
          <LabelText>{getLocale('braveWalletValue')}:</LabelText>
          <DetailText>{withdrawal.value}</DetailText>
        </DetailColumn>
        <VerticalDivider />
      </DetailColumn>
    ))}
  </>
)

const CardanoTxDetailScriptDataHash = ({
  scriptDataHash,
}: {
  scriptDataHash: string | undefined
}) => {
  if (!scriptDataHash) {
    return null
  }
  return (
    <DetailColumn>
      <LabelText>{getLocale('braveWalletScriptDataHash')}:</LabelText>
      <DetailText>{scriptDataHash}</DetailText>
    </DetailColumn>
  )
}

const CardanoTxDetailCollateral = ({
  collateral,
}: {
  collateral: CardanoTxDetailsUtxo[] | undefined
}) => (
  <>
    {collateral?.map((input, index) => (
      <CardanoTxDetailsUtxoItem
        key={'collateral' + index}
        label={getLocale('braveWalletCollateral')}
        utxo={input}
      />
    ))}
  </>
)

const CardanoTxDetailCollateralReturn = ({
  collateralReturn,
}: {
  collateralReturn: CardanoTxDetailsOutput | undefined
}) => {
  if (!collateralReturn) {
    return null
  }
  return (
    <DetailColumn gap='8px'>
      <LabelText>{getLocale('braveWalletCollateralReturn')}:</LabelText>
      <CardanoTxDetailsOutputItem
        label={getLocale('braveWalletCollateralReturn')}
        output={collateralReturn}
      />
    </DetailColumn>
  )
}

const CardanoTxDetailTotalCollateral = ({
  totalCollateral,
}: {
  totalCollateral: string | undefined
}) => {
  if (!totalCollateral) {
    return null
  }
  return (
    <DetailColumn>
      <LabelText>{getLocale('braveWalletTotalCollateral')}:</LabelText>
      <DetailText>{totalCollateral}</DetailText>
    </DetailColumn>
  )
}

const CardanoTxDetailsView = ({ details }: { details: string }) => {
  const parsed = React.useMemo(() => parseCardanoTxDetails(details), [details])

  if (!parsed) {
    return <DetailText style={{ whiteSpace: 'pre-wrap' }}>{details}</DetailText>
  }

  return (
    <Column
      width='100%'
      gap='8px'
      alignItems='flex-start'
    >
      <CardanoTxDetailInputs inputs={parsed.inputs} />
      <CardanoTxDetailOutputs outputs={parsed.outputs} />
      <CardanoTxDetailMint mint={parsed.mint} />
      <CardanoTxDetailWithdrawals withdrawals={parsed.withdrawals} />
      <CardanoTxDetailScriptDataHash scriptDataHash={parsed.scriptDataHash} />
      <CardanoTxDetailCollateral collateral={parsed.collateral} />
      <CardanoTxDetailCollateralReturn
        collateralReturn={parsed.collateralReturn}
      />
      <CardanoTxDetailTotalCollateral
        totalCollateral={parsed.totalCollateral}
      />
    </Column>
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
          <MessageBox width='100%'>
            {selectedTab === 'rawTransaction' ? (
              <DetailText style={{ whiteSpace: 'pre-wrap' }}>
                {selectedRequest.rawTxData}
              </DetailText>
            ) : (
              <CardanoTxDetailsView details={selectedRequest.detailsJson} />
            )}
          </MessageBox>
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
