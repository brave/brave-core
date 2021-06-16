// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { CaratLeftIcon } from 'brave-ui/components/icons'

import {
  ActionButton,
  BackArrow,
  Box,
  FlexItem,
  PlainButton,
  Text
} from './style'

import QRIcon from './assets/icons/qr-code.png'
import IconAsset from '../../../widgets/shared/iconAsset'
import { currencyNames } from '../../../widgets/shared/data'
import { getLocale } from '../../../../common/locale'

async function copyToClipboard (address: string) {
  try {
    await navigator.clipboard.writeText(address)
  } catch (e) {
    console.log(`Could not copy address ${e.toString()}`)
  }
}

interface Props {
  assetAddress: string
  assetQR: string
  base: string
  handleBackClick: () => void
}

export default function AssetDepositView ({
  assetAddress,
  assetQR,
  base,
  handleBackClick
}: Props) {
  const [showQR, setShowQR] = React.useState(false)

  return showQR
  ? (
    <Box isFlex={true} column={true} $p={10}>
      <img src={assetQR} />
      <ActionButton onClick={setShowQR.bind(this, false)} $mt={10} small={true} light={true} isFullWidth={false}>
        Done
      </ActionButton>
    </Box>
  ) : (
    <Box $p={0}>
      <FlexItem
        hasPadding={true}
        isFlex={true}
        isFullWidth={true}
        hasBorder={true}
      >
        <FlexItem>
          <BackArrow onClick={handleBackClick}>
            <CaratLeftIcon />
          </BackArrow>
        </FlexItem>
        <FlexItem $pr={5}>
          <IconAsset iconKey={base.toLowerCase()} />
        </FlexItem>
        <FlexItem flex={1}>
          <Text>{base}</Text>
          <Text small={true} textColor='light'>
            {currencyNames[base]}
          </Text>
        </FlexItem>
        <FlexItem $pl={5}>
          <PlainButton onClick={setShowQR.bind(this, true)}>
            <img width={25} src={QRIcon} />
          </PlainButton>
        </FlexItem>
      </FlexItem>
      <FlexItem
        $p='0.5em'
        isFullWidth={true}
      >
        <Text $fontSize={13} weight={600}>{base} Address</Text>
        <Text $fontSize={13} breakWord={true}>{assetAddress}</Text>
        <ActionButton onClick={copyToClipboard.bind(this, assetAddress)} $mt={5} $mb={15} small={true} light={true} isFullWidth={false}>
          {getLocale('cryptoDotComWidgetCopyAddress')}
        </ActionButton>
        <Text $fontSize={13} weight={600}>{getLocale('cryptoDotComWidgetSendCaveatHeading', { currency: base })}</Text>
        <Text $fontSize={13}>{getLocale('cryptoDotComWidgetSendCaveatBody', { currency: base })}</Text>
      </FlexItem>
    </Box>
  )
}
