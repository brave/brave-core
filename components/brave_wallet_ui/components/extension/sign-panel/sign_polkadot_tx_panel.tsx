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
  CallSummaryBox,
  CallLabel,
  CallText,
  DetailsCollapse,
  DetailsCollapseTitle,
  PayloadJson,
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

interface DecodedCall {
  // The call being made, as `method(section)`.
  name: string
  // The whole `toHuman()` tree, pretty-printed for the details pane.
  json: string
}

/**
 * Reads the browser's `Call.toHuman()` JSON into the two things the panel
 * shows. Returns undefined when the payload is not a shape we can describe,
 * which is what gates the warning and the Sign button: polkadot-js leaves
 * `method`/`section` unset for a call whose index it cannot find in the
 * chain's metadata, and naming no call at all is indistinguishable, to the
 * user, from naming the wrong one.
 */
const parseDecodedCall = (asHuman: string): DecodedCall | undefined => {
  try {
    const parsed: unknown = JSON.parse(asHuman)
    if (typeof parsed !== 'object' || parsed === null) {
      return undefined
    }

    const { method, section } = parsed as Record<string, unknown>
    if (typeof method !== 'string' || typeof section !== 'string') {
      return undefined
    }

    return {
      name: `${method}(${section})`,
      json: JSON.stringify(parsed, null, 2),
    }
  } catch {
    return undefined
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
  const [isDetailsOpen, setIsDetailsOpen] = React.useState(false)

  // computed
  const decodedCall = React.useMemo(
    () => (details ? parseDecodedCall(details.asHuman) : undefined),
    [details],
  )

  // Shown only on the dead-end path below, so the user still sees something
  // when the browser could not describe the call. Signing stays blocked.
  const rawPayload = React.useMemo(
    () => formatJson(selectedRequest.rawPayloadJson),
    [selectedRequest.rawPayloadJson],
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
  // something they were never shown. Keyed off the decode rather than off
  // `details` so a response carrying an undescribable payload stays gated too.
  const isSignDisabled = isSigningDisabled || !decodedCall

  // A named call is a description, so the generic "we cannot tell you what this
  // does" warning would be noise in front of it. A missing fee does not count
  // as a failure to describe: the call still tells the user what they are
  // agreeing to, and it renders as '--' below. The decode is also still in
  // flight on first paint, and skeletons beat flashing a red warning that
  // resolves into a summary a moment later.
  const showSummary = isFetchingDetails || decodedCall !== undefined

  // Past the warning there is nothing left to approve, so the button becomes
  // Sign (disabled) rather than another Continue.
  const isSignAction = showSummary || signStep === SignDataSteps.SignData

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
          originInfo={selectedRequest.dappOriginInfo}
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
      {showSummary && (
        <Column
          fullWidth
          justifyContent='flex-start'
        >
          <CallSummaryBox>
            <CallLabel>
              {getLocale(S.BRAVE_WALLET_TRANSACTION_DETAIL_BOX_FUNCTION)}
            </CallLabel>
            {isFetchingDetails ? (
              <LoadingSkeleton
                width='100%'
                height={20}
              />
            ) : (
              <CallText>{decodedCall?.name}</CallText>
            )}
          </CallSummaryBox>
          <Row
            justifyContent='space-between'
            padding='0px 4px 8px 4px'
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
          {decodedCall && (
            <DetailsCollapse
              isOpen={isDetailsOpen}
              onToggle={() => setIsDetailsOpen((prev) => !prev)}
            >
              <DetailsCollapseTitle slot='title'>
                {getLocale(S.BRAVE_WALLET_DETAILS)}
              </DetailsCollapseTitle>
              <PayloadJson>{decodedCall.json}</PayloadJson>
            </DetailsCollapse>
          )}
        </Column>
      )}
      {!showSummary && signStep === SignDataSteps.SignRisk && (
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
      {!showSummary && signStep === SignDataSteps.SignData && (
        <MessageBox width='100%'>
          <DetailText style={{ whiteSpace: 'pre-wrap' }}>
            {rawPayload}
          </DetailText>
        </MessageBox>
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
            buttonType={isSignAction ? 'sign' : 'danger'}
            text={
              isSignAction
                ? getLocale(S.BRAVE_WALLET_SIGN_TRANSACTION_BUTTON)
                : getLocale(S.BRAVE_WALLET_BUTTON_CONTINUE)
            }
            onSubmit={isSignAction ? onSign : onAcceptSigningRisks}
            disabled={isSignAction ? isSignDisabled : isSigningDisabled}
          />
        </SignPanelButtonRow>
      </Column>
    </StyledWrapper>
  )
}

export default SignPolkadotTxPanel
