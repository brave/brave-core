// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { getLocale, splitStringForTag } from '../../../../common/locale'

// Components
import { NavButton } from '../buttons/nav-button/index'
import { PanelTab } from '../panel-tab/index'
import { CreateSiteOrigin } from '../../shared/create-site-origin/index'

// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  AccountNameText,
  PanelTitle,
  MessageBox,
  MessageText,
  ButtonRow,
  DecryptButton
} from './style'

import { TabRow, URLText } from '../shared-panel-styles'

// Hooks
import { useAccountOrb } from '../../../common/hooks/use-orb'
import { useAccountQuery } from '../../../common/slices/api.slice.extra'
import {
  useProcessPendingDecryptRequestMutation,
  useProcessPendingGetEncryptionPublicKeyRequestMutation
} from '../../../common/slices/api.slice'

export interface ProvidePubKeyPanelProps {
  payload: BraveWallet.GetEncryptionPublicKeyRequest
}

export function ProvidePubKeyPanel({ payload }: ProvidePubKeyPanelProps) {
  // queries
  const { account } = useAccountQuery(payload.accountId)

  // mutations
  const [processGetEncryptionPublicKeyRequest] =
    useProcessPendingGetEncryptionPublicKeyRequestMutation()

  const orb = useAccountOrb(account)

  const descriptionString = getLocale(
    'braveWalletProvideEncryptionKeyDescription'
  ).replace('$url', payload.originInfo.originSpec)
  const { duringTag, afterTag } = splitStringForTag(descriptionString)

  // methods
  const onProvideOrAllow = async () => {
    await processGetEncryptionPublicKeyRequest({
      requestId: payload.requestId,
      approved: true
    }).unwrap()
  }

  const onCancel = async (requestId: string) => {
    await processGetEncryptionPublicKeyRequest({
      requestId,
      approved: false
    }).unwrap()
  }

  // render
  return (
    <StyledWrapper>
      <AccountCircle orb={orb} />
      <AccountNameText>
        {reduceAccountDisplayName(account?.name ?? '', 14)}
      </AccountNameText>
      <PanelTitle>
        {getLocale('braveWalletProvideEncryptionKeyTitle')}
      </PanelTitle>
      <TabRow>
        <PanelTab
          isSelected={true}
          text={getLocale('braveWalletSignTransactionMessageTitle')}
        />
      </TabRow>
      <MessageBox needsCenterAlignment={false}>
        <MessageText>
          <CreateSiteOrigin
            originSpec={duringTag ?? ''}
            eTldPlusOne={payload.originInfo.eTldPlusOne}
          />
          {afterTag}
        </MessageText>
      </MessageBox>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletButtonCancel')}
          onSubmit={() => onCancel(payload.requestId)}
        />
        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletProvideEncryptionKeyButton')}
          onSubmit={onProvideOrAllow}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

interface DecryptRequestPanelProps {
  payload: BraveWallet.DecryptRequest
}

export function DecryptRequestPanel({ payload }: DecryptRequestPanelProps) {
  // state
  const [isDecrypted, setIsDecrypted] = React.useState<boolean>(false)

  // queries
  const { account } = useAccountQuery(payload.accountId)

  // mutations
  const [processDecryptRequest] = useProcessPendingDecryptRequestMutation()

  // custom hooks
  const orb = useAccountOrb(account)

  // methods
  const onAllow = async () => {
    await processDecryptRequest({
      requestId: payload.requestId,
      approved: true
    }).unwrap()
  }

  const onCancel = async () => {
    await processDecryptRequest({
      requestId: payload.requestId,
      approved: false
    }).unwrap()
  }

  const onDecryptMessage = () => {
    setIsDecrypted(true)
  }

  return (
    <StyledWrapper>
      <AccountCircle orb={orb} />
      <AccountNameText>
        {reduceAccountDisplayName(account?.name ?? '', 14)}
      </AccountNameText>
      <URLText>
        <CreateSiteOrigin
          originSpec={payload.originInfo.originSpec}
          eTldPlusOne={payload.originInfo.eTldPlusOne}
        />
      </URLText>
      <PanelTitle>
        {getLocale('braveWalletReadEncryptedMessageTitle')}
      </PanelTitle>
      <TabRow>
        <PanelTab
          isSelected={true}
          text={getLocale('braveWalletSignTransactionMessageTitle')}
        />
      </TabRow>
      <MessageBox needsCenterAlignment={!isDecrypted}>
        {!isDecrypted ? (
          <DecryptButton onClick={onDecryptMessage}>
            {getLocale('braveWalletReadEncryptedMessageDecryptButton')}
          </DecryptButton>
        ) : (
          <MessageText>{payload.unsafeMessage}</MessageText>
        )}
      </MessageBox>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletButtonCancel')}
          onSubmit={onCancel}
        />
        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletReadEncryptedMessageButton')}
          onSubmit={onAllow}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}
