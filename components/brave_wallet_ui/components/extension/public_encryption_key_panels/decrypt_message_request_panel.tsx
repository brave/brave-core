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
  useProcessPendingDecryptRequestMutation, //
} from '../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '$web-common/locale'

// Components
import {
  CreateAccountIcon, //
} from '../../shared/create-account-icon/create-account-icon'
import { CreateSiteOrigin } from '../../shared/create-site-origin'

// Styled Components
import {
  StyledWrapper,
  HeaderText,
  AccountNameText,
  Title,
  URLText,
  MessageContainer,
  MessageContainerTitle,
  MessageBox,
  MessageText,
  DecryptMessageBox,
} from './public_encyrption_key_panels.style'
import { Row, Column, VerticalSpace } from '../../shared/style'

interface Props {
  payload: BraveWallet.DecryptRequest
}

export function DecryptMessageRequestPanel(props: Props) {
  const { payload } = props

  // State
  const [isDecrypted, setIsDecrypted] = React.useState<boolean>(false)

  // Queries
  const { account } = useAccountQuery(payload.accountId)

  // Mutations
  const [processDecryptRequest] = useProcessPendingDecryptRequestMutation()

  // Methods
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

  // Render
  return (
    <StyledWrapper
      fullWidth={true}
      fullHeight={true}
      justifyContent='space-between'
    >
      <Column fullWidth={true}>
        <Row padding='18px'>
          <HeaderText textColor='primary'>
            {getLocale('braveWalletReadEncryptedMessageDecryptButton')}
          </HeaderText>
        </Row>
        <Column
          padding='16px'
          gap='20px'
          fullWidth={true}
        >
          <Column
            gap='8px'
            fullWidth={true}
          >
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
            <VerticalSpace space='12px' />
            <URLText textColor='secondary'>
              <CreateSiteOrigin
                originSpec={payload.originInfo.originSpec}
                eTldPlusOne={payload.originInfo.eTldPlusOne}
              />
            </URLText>
          </Column>
          <Title textColor='primary'>
            {getLocale('braveWalletReadEncryptedMessageTitle')}
          </Title>
          {isDecrypted ? (
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
                alignItems='flex-start'
                width='100%'
              >
                <MessageText
                  textColor='primary'
                  textAlign='left'
                >
                  {payload.unsafeMessage}
                </MessageText>
              </MessageBox>
            </MessageContainer>
          ) : (
            <DecryptMessageBox
              fullWidth={true}
              alignItems='center'
              justifyContent='center'
            >
              <div>
                <Button
                  onClick={onDecryptMessage}
                  kind='plain'
                  size='small'
                >
                  {getLocale('braveWalletReadEncryptedMessageDecryptButton')}
                </Button>
              </div>
            </DecryptMessageBox>
          )}
        </Column>
      </Column>
      <Row
        padding='16px'
        gap='16px'
      >
        <Button
          kind='outline'
          onClick={onCancel}
        >
          {getLocale('braveWalletButtonCancel')}
        </Button>
        <Button onClick={onAllow}>
          {getLocale('braveWalletReadEncryptedMessageButton')}
        </Button>
      </Row>
    </StyledWrapper>
  )
}
