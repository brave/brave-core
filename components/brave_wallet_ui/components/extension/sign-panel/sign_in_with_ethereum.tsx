// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet } from '../../../constants/types'

// Queries
import { useAccountQuery } from '../../../common/slices/api.slice.extra'

// Components
import CreateSiteOrigin from '../../shared/create-site-origin/index'
import {
  CreateAccountIcon //
} from '../../shared/create-account-icon/create-account-icon'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'

// Styles
import {
  StyledWrapper,
  Title,
  Description,
  FavIcon,
  OriginName,
  MessageBox,
  MessageText,
  IconButton,
  CloseIcon,
  DetailsKeyText,
  DetailsInfoText,
  CodeBlock,
  URLText
} from './sign_in_with_ethereum.style'
import {
  Row,
  Column,
  VerticalDivider,
  VerticalSpace,
  HorizontalSpace,
  ScrollableColumn
} from '../../shared/style'

// Helpers
export const getKeyLocale = (key: string) => {
  const capitalized = key.charAt(0).toUpperCase() + key.slice(1)
  const localeString = `braveWallet${capitalized}`
  return getLocale(localeString) === localeString
    ? key
    : getLocale(localeString)
}

const getFormattedKeyValue = (key: string, data?: BraveWallet.SIWEMessage) => {
  if (!data || !data[key]) {
    return ''
  }

  if (key === 'origin') {
    return (
      <CodeBlock>
        {JSON.stringify(
          Object.fromEntries(
            Object.entries(data[key]).filter(
              ([k]) => !k.includes('nonceIfOpaque')
            )
          )
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
  onSignIn: () => void
  onCancel: () => void
}

export const SignInWithEthereum = (props: Props) => {
  const { data, onSignIn, onCancel } = props

  // State
  const [showDetails, setShowDetails] = React.useState<boolean>(false)

  // Queries
  const { account } = useAccountQuery(data.accountId || skipToken)

  // Computed
  const dataKeys = !data.signData.ethSiweData
    ? []
    : Object.keys(data.signData.ethSiweData).filter(
        (key) => data.signData?.ethSiweData?.[key] !== null
      )

  const hasMessageAndResource =
    data.signData.ethSiweData?.statement && data.signData.ethSiweData?.resources

  if (showDetails) {
    return (
      <StyledWrapper>
        <>
          <Row
            padding='14px 16px 13px 16px'
            justifyContent='space-between'
          >
            <Title
              isBold={true}
              textSize='18px'
            >
              {getLocale('braveWalletSeeDetails')}
            </Title>
            <IconButton onClick={() => setShowDetails(false)}>
              <CloseIcon />
            </IconButton>
          </Row>
          <VerticalDivider />
        </>
        <ScrollableColumn
          fullWidth={true}
          fullHeight={true}
          padding='0px 16px'
        >
          {dataKeys.map((objectKey: string, key: number) => (
            <React.Fragment key={objectKey}>
              <Row
                justifyContent='flex-start'
                padding='8px 0px'
              >
                <DetailsKeyText
                  textSize='14px'
                  isBold={true}
                >
                  {getKeyLocale(objectKey)}
                </DetailsKeyText>
                <DetailsInfoText
                  textSize='12px'
                  isBold={false}
                >
                  {getFormattedKeyValue(objectKey, data.signData.ethSiweData)}
                </DetailsInfoText>
              </Row>
              {dataKeys.length - 1 !== key && (
                <Row>
                  <VerticalDivider />
                </Row>
              )}
            </React.Fragment>
          ))}
        </ScrollableColumn>
      </StyledWrapper>
    )
  }

  return (
    <StyledWrapper>
      <>
        <Row padding='16px 16px 11px 16px'>
          <Title
            isBold={true}
            textSize='18px'
          >
            {getLocale('braveWalletSignInWithBraveWallet')}
          </Title>
        </Row>
        <VerticalDivider />
      </>
      <Column
        fullWidth={true}
        padding='16px 16px 0px 16px'
      >
        <FavIcon
          src={`chrome://favicon/size/64@1x/${data.originInfo.originSpec}`}
        />
        <OriginName
          isBold={true}
          textSize='16px'
        >
          {data.originInfo.eTldPlusOne}
        </OriginName>
        <Description
          isBold={true}
          textSize='12px'
        >
          <CreateSiteOrigin
            originSpec={data.originInfo.originSpec}
            eTldPlusOne={data.originInfo.eTldPlusOne}
          />
        </Description>
        <VerticalSpace space='8px' />
        <VerticalDivider />
        <VerticalSpace space='8px' />
        <MessageBox>
          {account && (
            <Row
              padding='8px 9px 12px 9px'
              justifyContent='flex-start'
            >
              <CreateAccountIcon
                size='medium'
                account={account}
                marginRight={18}
              />
              <Column alignItems='flex-start'>
                <Title
                  isBold={true}
                  textSize='14px'
                >
                  {account.name}
                </Title>
                <Description
                  isBold={false}
                  textSize='12px'
                >
                  {reduceAddress(account.address)}
                </Description>
              </Column>
            </Row>
          )}
          <Column
            fullWidth={true}
            padding='0px 16px 16px 16px'
          >
            <MessageText
              textSize='14px'
              isBold={false}
            >
              {getLocale('braveWalletSignInWithBraveWalletMessage').replaceAll(
                '$1',
                data.originInfo.eTldPlusOne
              )}
            </MessageText>
            <Row justifyContent='flex-start'>
              <Row width='unset'>
                <Button
                  kind='plain'
                  onClick={() => setShowDetails(true)}
                >
                  {getLocale('braveWalletSeeDetails')}
                </Button>
              </Row>
            </Row>
          </Column>
          {hasMessageAndResource && (
            <Column
              fullWidth={true}
              padding='0px 16px 8px 16px'
              alignItems='flex-start'
            >
              <VerticalDivider />
              <VerticalSpace space='16px' />
              <Title
                isBold={true}
                textSize='14px'
              >
                {getLocale('braveWalletSignTransactionMessageTitle')}:
              </Title>
              <VerticalSpace space='4px' />
              <URLText
                textSize='14px'
                isBold={false}
              >
                {data.signData.ethSiweData?.statement ?? ''}
              </URLText>
              <VerticalSpace space='16px' />
              <Title
                isBold={true}
                textSize='14px'
              >
                {getLocale('braveWalletResources')}:
              </Title>
              <VerticalSpace space='4px' />
              {data.signData.ethSiweData?.resources?.map((resource) => (
                <React.Fragment key={resource.url}>
                  <URLText
                    textSize='14px'
                    isBold={false}
                  >
                    {resource.url}
                  </URLText>
                  <VerticalSpace space='4px' />
                </React.Fragment>
              ))}
            </Column>
          )}
        </MessageBox>
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
