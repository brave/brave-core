// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Assets
import ChecksumInfoGraphic from '../../assets/checksum-info-graphic.png'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Styled Components
import { Wrapper, InfoColumn, InfoGraphic, Link } from './checksum_info.style'
import {
  Column,
  Text,
  VerticalSpacer
} from '../../../../../components/shared/style'

export const ChecksumInfo = () => {
  return (
    <Wrapper
      fullWidth={true}
      justifyContent='flex-start'
    >
      <Column
        fullWidth={true}
        justifyContent='center'
        alignItems='center'
        padding='0px 40px'
      >
        <Text
          textSize='22px'
          textColor='primary'
          isBold={true}
        >
          {getLocale('braveWalletChecksumModalTitle')}
        </Text>
        <VerticalSpacer space={16} />
        <Text
          textSize='14px'
          textColor='secondary'
          isBold={false}
        >
          {getLocale('braveWalletChecksumModalDescription')}
        </Text>
      </Column>
      <VerticalSpacer space={24} />
      <InfoColumn
        fullWidth={true}
        justifyContent='center'
        alignItems='flex-start'
        padding='24px 40px'
      >
        <Text
          textSize='14px'
          textColor='primary'
          isBold={true}
        >
          {getLocale('braveWalletChecksumModalStepOneTitle')}{' '}
          <Link
            href='https://etherscan.io'
            target='_blank'
            rel='noopener noreferrer'
          >
            https://etherscan.io
          </Link>
        </Text>
        <VerticalSpacer space={8} />
        <Text
          textSize='12px'
          textColor='secondary'
          isBold={false}
          textAlign='left'
        >
          {getLocale('braveWalletChecksumModalStepOneDescription')}
        </Text>
        <VerticalSpacer space={24} />
        <Text
          textSize='14px'
          textColor='primary'
          isBold={true}
        >
          {getLocale('braveWalletChecksumModalStepTwoTitle')}
        </Text>
        <VerticalSpacer space={8} />
        <Text
          textSize='12px'
          textColor='secondary'
          isBold={false}
          textAlign='left'
        >
          {getLocale('braveWalletChecksumModalStepTwoDescription')}
        </Text>
        <VerticalSpacer space={8} />
        <InfoGraphic src={ChecksumInfoGraphic} />
      </InfoColumn>
      <Column
        fullWidth={true}
        justifyContent='center'
        alignItems='center'
        padding='52px 0px'
      >
        <Text
          textSize='14px'
          textColor='secondary'
          isBold={true}
        >
          {getLocale('braveWalletChecksumModalNeedHelp')}{' '}
          <Link
            href='https://support.brave.com/hc/en-us/articles/4415497656461-Brave-Wallet-FAQ'
            target='_blank'
            rel='noopener noreferrer'
          >
            {getLocale('braveWalletHelpCenter')}
          </Link>
        </Text>
      </Column>
    </Wrapper>
  )
}
