// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { AddressMessageInfo } from '../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Styled Components
import {
  Wrapper,
  LearnMoreLink,
  HowToSolveButton,
  ErrorIcon
} from './address-message.style'
import {
  Column,
  Row,
  Text,
  LeoSquaredButton
} from '../../../../../components/shared/style'

interface Props {
  addressMessageInfo: AddressMessageInfo
  onClickHowToSolve?: () => void
  onClickEnableENSOffchain?: () => void
}

export const AddressMessage = (props: Props) => {
  const { addressMessageInfo, onClickHowToSolve, onClickEnableENSOffchain } =
    props

  return (
    <Wrapper
      alignItems='flex-start'
      justifyContent='flex-start'
      padding='16px'
      type={addressMessageInfo.type}
    >
      {addressMessageInfo.type && addressMessageInfo.type !== 'info' && (
        <ErrorIcon
          name={
            addressMessageInfo.type === 'error'
              ? 'warning-circle-filled'
              : 'warning-triangle-filled'
          }
          type={addressMessageInfo.type}
        />
      )}
      <Column
        alignItems='flex-start'
        justifyContent='flex-start'
      >
        {addressMessageInfo.title && (
          <Text
            textSize='12px'
            textColor={addressMessageInfo.type ?? 'info'}
            isBold={true}
            textAlign='left'
          >
            {getLocale(addressMessageInfo.title)}
          </Text>
        )}
        {addressMessageInfo.description && (
          <>
            <Row>
              <Text
                textSize='12px'
                textColor={addressMessageInfo.type ?? 'info'}
                isBold={false}
                textAlign='left'
              >
                {getLocale(addressMessageInfo.description).replace(
                  '$1',
                  addressMessageInfo.placeholder || ''
                )}{' '}
                {addressMessageInfo.url && (
                  <LearnMoreLink
                    href={addressMessageInfo.url}
                    target='_blank'
                    rel='noopener noreferrer'
                  >
                    {getLocale('braveWalletLearnMore')}
                  </LearnMoreLink>
                )}
                {onClickHowToSolve && (
                  <HowToSolveButton onClick={onClickHowToSolve}>
                    {getLocale('braveWalletHowToSolve')}
                  </HowToSolveButton>
                )}
              </Text>
            </Row>
          </>
        )}
        {onClickEnableENSOffchain && (
          <Row
            alignItems='flex-start'
            justifyContent='flex-start'
            margin='16px 0px 0px 0px'
            width='unset'
          >
            <LeoSquaredButton onClick={onClickEnableENSOffchain}>
              {getLocale('braveWalletEnsOffChainButton')}
            </LeoSquaredButton>
          </Row>
        )}
      </Column>
    </Wrapper>
  )
}

export default AddressMessage
