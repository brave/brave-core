// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Queries
import {
  useGetActiveOriginQuery,
  useGetSelectedDappAccountsQuery,
} from '../../../common/slices/api.slice'

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
import {
  FavIcon,
  DomainText,
  DomainTextContainer, //
} from './connections.style'
import { Column } from '../../shared/style'
import { VerifiedLabel } from '../../shared/verified_label/verified_label'

export const Connections = () => {
  // Queries
  const { data: activeOrigin = { eTldPlusOne: '', originSpec: '' } } =
    useGetActiveOriginQuery()
  const { data: dappsAccounts } = useGetSelectedDappAccountsQuery()

  // Redux
  const isCardanoDappSupportEnabled = useSafeWalletSelector(
    WalletSelectors.isCardanoDappSupportEnabled,
  )

  // Hooks
  const { isDAppVerified } = useIsDAppVerified(activeOrigin)

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
        <DomainTextContainer
          gap='4px'
          margin='0px 0px 24px 0px'
          padding='0px 24px'
        >
          <DomainText
            textSize='16px'
            isBold={true}
            textColor='primary'
          >
            {activeOrigin.eTldPlusOne}
          </DomainText>
          <DomainText
            textSize='14px'
            isBold={false}
            textColor='tertiary'
          >
            <CreateSiteOrigin
              originSpec={activeOrigin.originSpec}
              eTldPlusOne={activeOrigin.eTldPlusOne}
            />
          </DomainText>
          {isDAppVerified && <VerifiedLabel />}
        </DomainTextContainer>
        <Column
          gap='16px'
          width='100%'
        >
          <ConnectionSection
            coin={BraveWallet.CoinType.ETH}
            selectedAccountId={dappsAccounts?.ethAccountId}
          />
          <ConnectionSection
            coin={BraveWallet.CoinType.SOL}
            selectedAccountId={dappsAccounts?.solAccountId}
          />
          {isCardanoDappSupportEnabled && (
            <ConnectionSection
              coin={BraveWallet.CoinType.ADA}
              selectedAccountId={dappsAccounts?.adaAccountId}
            />
          )}
        </Column>
      </Column>
    </WalletPageWrapper>
  )
}
