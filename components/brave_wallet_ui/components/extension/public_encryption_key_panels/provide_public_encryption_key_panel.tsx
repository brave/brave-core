// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Label from '@brave/leo/react/label'
import Tooltip from '@brave/leo/react/tooltip'

// Hooks
import {
  useAccountQuery, //
} from '../../../common/slices/api.slice.extra'
import {
  useProcessPendingGetEncryptionPublicKeyRequestMutation, //
} from '../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale, formatLocale } from '$web-common/locale'

// Components
import { CreateSiteOrigin } from '../../shared/create-site-origin/index'
import {
  CreateAccountIcon, //
} from '../../shared/create-account-icon/create-account-icon'

// Styled Components
import {
  StyledWrapper,
  HeaderText,
  AccountNameText,
  Title,
  MessageContainer,
  MessageContainerTitle,
  MessageBox,
  MessageText,
} from './public_encyrption_key_panels.style'
import { Row, Column } from '../../shared/style'

interface Props {
  payload: BraveWallet.GetEncryptionPublicKeyRequest
}

export function ProvidePublicEncryptionKeyPanel(props: Props) {
  const { payload } = props

  // Queries
  const { account } = useAccountQuery(payload.accountId)

  // Mutations
  const [processGetEncryptionPublicKeyRequest] =
    useProcessPendingGetEncryptionPublicKeyRequestMutation()

  const description = formatLocale(
    'braveWalletProvideEncryptionKeyDescription',
    {
      $1: (
        <CreateSiteOrigin
          originSpec={payload.originInfo.originSpec}
          eTldPlusOne={payload.originInfo.eTldPlusOne}
        />
      ),
    },
  )

  // Methods
  const onProvide = async () => {
    await processGetEncryptionPublicKeyRequest({
      requestId: payload.requestId,
      approved: true,
    }).unwrap()
  }

  const onCancel = async (requestId: string) => {
    await processGetEncryptionPublicKeyRequest({
      requestId,
      approved: false,
    }).unwrap()
  }

  // Render
  return (
    <StyledWrapper
      fullWidth={true}
      fullHeight={true}
      justifyContent='space-between'
    >
      <Column>
        <Row padding='18px'>
          <HeaderText textColor='primary'>
            {getLocale('braveWalletPublicEncryptionKey')}
          </HeaderText>
        </Row>
        <Column
          padding='16px'
          gap='40px'
        >
          <Column gap='8px'>
            <CreateAccountIcon
              size='huge'
              account={account}
            />
            <AccountNameText textColor='secondary'>
              {account?.name ?? ''}
            </AccountNameText>
            <Tooltip text={account?.address ?? ''}>
              <Label>{reduceAddress(account?.address ?? '')}</Label>
            </Tooltip>
          </Column>
          <Title textColor='primary'>
            {getLocale('braveWalletProvideEncryptionKeyTitle')}
          </Title>
          <MessageContainer
            fullWidth={true}
            padding='0px 2px 2px 2px'
          >
            <Row padding='8px'>
              <MessageContainerTitle textColor='tertiary'>
                {getLocale('braveWalletSignTransactionMessageTitle')}
              </MessageContainerTitle>
            </Row>
            <MessageBox
              padding='8px 16px'
              justifyContent='flex-start'
            >
              <MessageText textColor='primary'>{description}</MessageText>
            </MessageBox>
          </MessageContainer>
        </Column>
      </Column>
      <Row
        padding='16px'
        gap='16px'
      >
        <Button
          kind='outline'
          onClick={() => onCancel(payload.requestId)}
        >
          {getLocale('braveWalletButtonCancel')}
        </Button>
        <Button onClick={onProvide}>
          {getLocale('braveWalletProvideEncryptionKeyButton')}
        </Button>
      </Row>
    </StyledWrapper>
  )
}
