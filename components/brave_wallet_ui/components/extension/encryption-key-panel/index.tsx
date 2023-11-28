// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { getLocale, splitStringForTag } from '../../../../common/locale'
import { PanelActions } from '../../../panel/actions'

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

export interface ProvidePubKeyPanelProps {
  payload: BraveWallet.GetEncryptionPublicKeyRequest
}

export function ProvidePubKeyPanel({ payload }: ProvidePubKeyPanelProps) {
  // redux
  const dispatch = useDispatch()

  // queries
  const { account } = useAccountQuery(payload.accountId)

  const orb = useAccountOrb(account)

  const descriptionString = getLocale(
    'braveWalletProvideEncryptionKeyDescription'
  ).replace('$url', payload.originInfo.originSpec)
  const { duringTag, afterTag } = splitStringForTag(descriptionString)

  // methods
  const onProvideOrAllow = () => {
    dispatch(
      PanelActions.getEncryptionPublicKeyProcessed({
        requestId: payload.requestId,
        approved: true
      })
    )
  }

  const onCancel = (requestId: string) => {
    dispatch(
      PanelActions.getEncryptionPublicKeyProcessed({
        requestId,
        approved: false
      })
    )
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
  // redux
  const dispatch = useDispatch()

  // state
  const [isDecrypted, setIsDecrypted] = React.useState<boolean>(false)

  // queries
  const { account } = useAccountQuery(payload.accountId)

  // custom hooks
  const orb = useAccountOrb(account)

  // methods
  const onAllow = () => {
    dispatch(
      PanelActions.decryptProcessed({
        requestId: payload.requestId,
        approved: true
      })
    )
  }

  const onCancel = () => {
    dispatch(
      PanelActions.decryptProcessed({
        requestId: payload.requestId,
        approved: false
      })
    )
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
