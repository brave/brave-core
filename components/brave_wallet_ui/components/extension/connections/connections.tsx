// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Queries
import { useGetActiveOriginQuery } from '../../../common/slices/api.slice'

// Hooks
import {
  useSafeWalletSelector, //
} from '../../../common/hooks/use-safe-selector'
import {
  useIsDAppVerified, //
} from '../../../common/hooks/use_is_dapp_verified'

// selectors
import { WalletSelectors } from '../../../common/selectors'

// Utils
import { getLocale } from '$web-common/locale'

// Components
import {
  WalletPageWrapper, //
} from '../../desktop/wallet-page-wrapper/wallet-page-wrapper'
import {
  DefaultPanelHeader, //
} from '../../desktop/card-headers/default-panel-header'
import { CreateSiteOrigin } from '../../shared/create-site-origin/index'
import {
  ConnectionSection, //
} from './components/connection_section/connection_section'

// Styled Components
import { FavIcon } from './connections.style'
import { Column, Text } from '../../shared/style'
import { VerifiedLabel } from '../../shared/verified_label/verified_label'

const CONNECTABLE_COIN_TYPES = [
  BraveWallet.CoinType.ETH,
  BraveWallet.CoinType.SOL,
  BraveWallet.CoinType.ADA,
]

export const Connections = () => {
  // Queries
  const { data: activeOrigin = { eTldPlusOne: '', originSpec: '' } } =
    useGetActiveOriginQuery()

  // Redux
  const isCardanoDappSupportEnabled = useSafeWalletSelector(
    WalletSelectors.isCardanoDappSupportEnabled,
  )

  // Hooks
  const isDAppVerified = useIsDAppVerified(activeOrigin)

  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      noCardPadding={true}
      useDarkBackground={true}
      isConnection={true}
      cardHeader={
        <DefaultPanelHeader title={getLocale('braveWalletConnections')} />
      }
    >
      <Column
        fullWidth={true}
        padding='8px 16px'
      >
        <FavIcon
          src={`chrome://favicon2?size=64&pageUrl=${encodeURIComponent(
            activeOrigin.originSpec,
          )}`}
        />
        <Column
          gap='4px'
          margin='0px 0px 24px 0px'
        >
          <Text
            textSize='16px'
            isBold={true}
            textColor='primary'
          >
            {activeOrigin.eTldPlusOne}
          </Text>
          <Text
            textSize='14px'
            isBold={false}
            textColor='tertiary'
          >
            <CreateSiteOrigin
              originSpec={activeOrigin.originSpec}
              eTldPlusOne={activeOrigin.eTldPlusOne}
            />
          </Text>
          {isDAppVerified && <VerifiedLabel />}
        </Column>
        <Column
          gap='16px'
          width='100%'
        >
          {CONNECTABLE_COIN_TYPES.filter(
            (coin) =>
              coin !== BraveWallet.CoinType.ADA || isCardanoDappSupportEnabled,
          ).map((coin) => (
            <ConnectionSection
              key={coin}
              coin={coin}
            />
          ))}
        </Column>
      </Column>
    </WalletPageWrapper>
  )
}
