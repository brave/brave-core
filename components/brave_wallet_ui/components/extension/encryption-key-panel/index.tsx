// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { getLocale } from '$web-common/locale'

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
  DecryptButton,
} from './style'

import { TabRow, URLText } from '../shared-panel-styles'

// Hooks
import { useAccountOrb } from '../../../common/hooks/use-orb'
import { useAccountQuery } from '../../../common/slices/api.slice.extra'
import {
  useProcessPendingDecryptRequestMutation, //
} from '../../../common/slices/api.slice'

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
      approved: true,
    }).unwrap()
  }

  const onCancel = async () => {
    await processDecryptRequest({
      requestId: payload.requestId,
      approved: false,
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
