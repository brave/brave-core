// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet, SignDataSteps } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'
import { Uint128ToBigInt } from '../../../utils/polkadot-utils'

// Hooks
import { useAccountOrb } from '../../../common/hooks/use-orb'
import {
  useProcessSignPolkadotTransaction, //
} from '../../../common/hooks/use_sign_polkadot_tx_queue'

// Components
import NavButton from '../buttons/nav-button/index'
import CreateSiteOrigin from '../../shared/create-site-origin/index'
import { TransactionQueueSteps } from '../confirm-transaction-panel/common/queue'
import {
  DetailText, //
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
  WarningBox,
  WarningText,
  LearnMoreButton,
  URLText,
  WarningIcon,
} from '../shared-panel-styles'

import { Tooltip } from '../../shared/tooltip/index'
import LoadingSkeleton from '../../shared/loading-skeleton'
import { Column, Row, Text } from '../../shared/style'

interface Props {
  selectedRequest: BraveWallet.SignPolkadotTransactionRequest
  isSigningDisabled: boolean
  network: BraveWallet.NetworkInfo
  queueNextSignTransaction: () => void
  signingAccount: BraveWallet.AccountInfo
  queueLength: number
  queueNumber: number
  // Null when the browser could not decode the request, which blocks signing.
  details: BraveWallet.PolkadotSignRequestDetails | null
  isFetchingDetails: boolean
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

const formatJson = (json: string): string => {
  try {
    return JSON.stringify(JSON.parse(json), null, 2)
  } catch {
    return json
  }
}

export const SignPolkadotTxPanel = ({
  selectedRequest,
  isSigningDisabled,
  network,
  queueLength,
  queueNextSignTransaction,
  queueNumber,
  signingAccount,
  details,
  isFetchingDetails,
}: Props) => {
  // custom hooks
  const orb = useAccountOrb(signingAccount)

  // state
  const [signStep, setSignStep] = React.useState<SignDataSteps>(
    SignDataSteps.SignRisk,
  )

  // computed
  // Fall back to the raw payload when the browser could not decode the call, so
  // the user still sees something. Signing stays blocked below.
  const payload = React.useMemo(
    () =>
      formatJson(details ? details.asHuman : selectedRequest.rawPayloadJson),
    [details, selectedRequest.rawPayloadJson],
  )

  const feeAmount = React.useMemo(() => {
    const partialFee = Uint128ToBigInt(details?.fee?.partialFee ?? undefined)
    if (partialFee === undefined) {
      return undefined
    }

    return new Amount(partialFee.toString())
      .divideByDecimals(network.decimals)
      .formatAsAsset(6, network.symbol)
  }, [details, network.decimals, network.symbol])

  // An undescribed request must not be signable: the user would be approving
  // something they were never shown.
  const isSignDisabled = isSigningDisabled || !details

  // methods
  const onAcceptSigningRisks = React.useCallback(() => {
    setSignStep(SignDataSteps.SignData)
  }, [])

  const { cancelSign: onCancelSign, sign: onSign } =
    useProcessSignPolkadotTransaction({
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
        {getLocale(S.BRAVE_WALLET_SIGN_TRANSACTION_TITLE)}
      </PanelTitle>
      {signStep === SignDataSteps.SignRisk && (
        <WarningBox warningType='danger'>
          <WarningTitleRow>
            <WarningIcon />
            <Text
              textColor='error'
              variant='small.semibold'
            >
              {getLocale(S.BRAVE_WALLET_SIGN_WARNING_TITLE)}
            </Text>
          </WarningTitleRow>
          <WarningText
            textColor='error'
            variant='small.regular'
          >
            {getLocale(S.BRAVE_WALLET_SIGN_WARNING)}
          </WarningText>
          <LearnMoreButton onClick={onClickLearnMore}>
            {getLocale(S.BRAVE_WALLET_ALLOW_ADD_NETWORK_LEARN_MORE_BUTTON)}
          </LearnMoreButton>
        </WarningBox>
      )}
      {signStep === SignDataSteps.SignData && (
        <>
          <MessageBox width='100%'>
            {isFetchingDetails ? (
              <LoadingSkeleton
                width='100%'
                height={80}
              />
            ) : (
              <DetailText style={{ whiteSpace: 'pre-wrap' }}>
                {payload}
              </DetailText>
            )}
          </MessageBox>
          <Row
            justifyContent='space-between'
            padding='8px 0px'
          >
            <Text
              textColor='tertiary'
              variant='small.regular'
            >
              {getLocale(S.BRAVE_WALLET_NETWORK_FEES)}
            </Text>
            {isFetchingDetails ? (
              <LoadingSkeleton width={64} />
            ) : (
              <Text
                textColor='primary'
                variant='small.semibold'
              >
                {feeAmount ?? '--'}
              </Text>
            )}
          </Row>
        </>
      )}
      <Column
        fullWidth
        gap={'8px'}
      >
        <SignPanelButtonRow>
          <NavButton
            buttonType='secondary'
            text={getLocale(S.BRAVE_WALLET_BUTTON_CANCEL)}
            onSubmit={onCancelSign}
            disabled={isSigningDisabled}
          />
          <NavButton
            buttonType={signStep === SignDataSteps.SignData ? 'sign' : 'danger'}
            text={
              signStep === SignDataSteps.SignData
                ? getLocale(S.BRAVE_WALLET_SIGN_TRANSACTION_BUTTON)
                : getLocale(S.BRAVE_WALLET_BUTTON_CONTINUE)
            }
            onSubmit={
              signStep === SignDataSteps.SignRisk
                ? onAcceptSigningRisks
                : onSign
            }
            disabled={
              signStep === SignDataSteps.SignData
                ? isSignDisabled
                : isSigningDisabled
            }
          />
        </SignPanelButtonRow>
      </Column>
    </StyledWrapper>
  )
}

export default SignPolkadotTxPanel
