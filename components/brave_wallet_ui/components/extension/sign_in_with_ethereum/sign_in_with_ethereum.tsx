// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Label from '@brave/leo/react/label'
import Tooltip from '@brave/leo/react/tooltip'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Mutations
import {
  useProcessSignMessageRequestMutation, //
} from '../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../constants/types'

// Queries
import {
  useAccountQuery, //
} from '../../../common/slices/api.slice.extra'

// Components
import {
  CreateAccountIcon, //
} from '../../shared/create-account-icon/create-account-icon'
import { OriginInfoCard } from '../origin_info_card/origin_info_card'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'

// Styles
import {
  StyledWrapper,
  HeaderText,
  Title,
  MessageBox,
  MessageText,
  IconButton,
  CloseIcon,
  DetailsKeyText,
  DetailsInfoText,
  CodeBlock,
  URLText,
  SectionTitle,
  DetailsWrapper,
  DetailsTitle,
  DetailsBox,
  DetailsContent,
} from './sign_in_with_ethereum.style'
import {
  Row,
  Column,
  VerticalDivider,
  VerticalSpace,
  HorizontalSpace,
} from '../../shared/style'

// Helpers
export const getKeyLocale = (key: string) => {
  const capitalized = key.charAt(0).toUpperCase() + key.slice(1)
  const localeString = `braveWallet${capitalized}`
  return getLocale(localeString) === localeString
    ? key
    : getLocale(localeString)
}

const getFormattedKeyValue = (
  key: keyof BraveWallet.SIWEMessage,
  data?: BraveWallet.SIWEMessage,
) => {
  if (!data?.[key]) {
    return ''
  }

  if (key === 'origin') {
    return (
      <CodeBlock>
        {JSON.stringify(
          Object.fromEntries(
            Object.entries(data[key]).filter(
              ([k]) => !k.includes('nonceIfOpaque'),
            ),
          ),
        )}
      </CodeBlock>
    )
  }
  if (key === 'uri') {
    return data[key].url
  }
  if (key === 'resources') {
    return data[key]?.map((resource) => {
      return (
        <React.Fragment key={resource.url}>
          <span>{resource.url}</span>
          <VerticalSpace space='4px' />
        </React.Fragment>
      )
    })
  }

  return data[key].toString()
}

interface Props {
  data: BraveWallet.SignMessageRequest
}

export const SignInWithEthereum = (props: Props) => {
  const { data } = props

  // mutations
  const [processSignMessageRequest] = useProcessSignMessageRequestMutation()

  // State
  const [showDetails, setShowDetails] = React.useState<boolean>(false)

  // Queries
  const { account } = useAccountQuery(data.accountId || skipToken)

  // Computed
  const dataKeys = !data.signData.ethSiweData
    ? []
    : Object.keys(data.signData.ethSiweData).filter(
        (key: keyof BraveWallet.SIWEMessage) =>
          data.signData?.ethSiweData?.[key] !== null,
      )

  const hasMessageAndResource =
    data.signData.ethSiweData?.statement && data.signData.ethSiweData?.resources

  // Methods
  const onSignIn = async () => {
    if (!account) {
      return
    }
    await processSignMessageRequest({
      approved: true,
      id: data.id,
    }).unwrap()
  }

  const onCancel = async () => {
    await processSignMessageRequest({
      approved: false,
      id: data.id,
    }).unwrap()
  }

  if (showDetails) {
    return (
      <DetailsWrapper
        fullWidth={true}
        fullHeight={true}
      >
        <Row
          padding='24px 16px'
          justifyContent='space-between'
        >
          <DetailsTitle textColor='primary'>
            {getLocale('braveWalletSeeDetails')}
          </DetailsTitle>
          <IconButton onClick={() => setShowDetails(false)}>
            <CloseIcon />
          </IconButton>
        </Row>
        <DetailsContent
          fullWidth={true}
          fullHeight={true}
          padding='0px 16px 16px 16px'
        >
          <DetailsBox
            fullWidth={true}
            fullHeight={true}
            padding='16px'
            gap='16px'
          >
            {dataKeys.map((objectKey: keyof BraveWallet.SIWEMessage) => (
              <Column
                key={objectKey}
                justifyContent='flex-start'
                alignItems='flex-start'
                fullWidth={true}
              >
                <DetailsKeyText textColor='tertiary'>
                  {getKeyLocale(objectKey)}
                </DetailsKeyText>
                <DetailsInfoText textColor='primary'>
                  {getFormattedKeyValue(objectKey, data.signData.ethSiweData)}
                </DetailsInfoText>
              </Column>
            ))}
          </DetailsBox>
        </DetailsContent>
      </DetailsWrapper>
    )
  }

  return (
    <StyledWrapper
      justifyContent='space-between'
      width='100%'
      height='100%'
    >
      <Column fullWidth={true}>
        <Row
          padding='18px'
          marginBottom='16px'
        >
          <HeaderText textColor='primary'>
            {getLocale('braveWalletSignInWithBraveWallet')}
          </HeaderText>
        </Row>

        {data.originInfo && (
          <Row
            justifyContent='flex-start'
            padding='0px 10px 24px 10px'
          >
            <OriginInfoCard
              origin={data.originInfo}
              noBackground={true}
            />
          </Row>
        )}

        <Column
          fullWidth={true}
          padding='0px 24px 0px 24px'
          gap='24px'
        >
          <MessageBox
            padding='16px'
            gap='16px'
            justifyContent='flex-start'
            alignItems='flex-start'
            fullWidth={true}
          >
            {account && (
              <Column
                fullWidth={true}
                gap='16px'
              >
                <Row justifyContent='flex-start'>
                  <CreateAccountIcon
                    size='huge'
                    account={account}
                    marginRight={8}
                  />
                  <Column
                    alignItems='flex-start'
                    gap='4px'
                  >
                    <Title textColor='secondary'>{account.name}</Title>
                    <Tooltip text={account.address}>
                      <Label>{reduceAddress(account.address)}</Label>
                    </Tooltip>
                  </Column>
                </Row>
                <VerticalDivider />
              </Column>
            )}
            <MessageText textColor='primary'>
              {getLocale('braveWalletSignInWithBraveWalletMessage').replaceAll(
                '$1',
                data.originInfo.eTldPlusOne,
              )}
            </MessageText>
            {hasMessageAndResource && (
              <Column
                fullWidth={true}
                alignItems='flex-start'
              >
                <VerticalDivider />
                <VerticalSpace space='16px' />
                <SectionTitle textColor='primary'>
                  {getLocale('braveWalletSignTransactionMessageTitle')}:
                </SectionTitle>
                <VerticalSpace space='4px' />
                <MessageText textColor='primary'>
                  {data.signData.ethSiweData?.statement ?? ''}
                </MessageText>
                <VerticalSpace space='16px' />
                <SectionTitle textColor='primary'>
                  {getLocale('braveWalletResources')}:
                </SectionTitle>
                <VerticalSpace space='4px' />
                {data.signData.ethSiweData?.resources?.map((resource) => (
                  <React.Fragment key={resource.url}>
                    <URLText textColor='primary'>{resource.url}</URLText>
                    <VerticalSpace space='4px' />
                  </React.Fragment>
                ))}
              </Column>
            )}
          </MessageBox>
          <Row width='unset'>
            <Button
              kind='plain'
              onClick={() => setShowDetails(true)}
            >
              {getLocale('braveWalletSeeDetails')}
            </Button>
          </Row>
        </Column>
      </Column>
      <Row padding='16px'>
        <Button
          kind='outline'
          onClick={onCancel}
        >
          {getLocale('braveWalletButtonCancel')}
        </Button>
        <HorizontalSpace space='16px' />
        <Button onClick={onSignIn}>{getLocale('braveWalletSignIn')}</Button>
      </Row>
    </StyledWrapper>
  )
}
