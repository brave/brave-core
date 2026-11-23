// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// Utils
import { WalletRoutes } from '$wallet/constants/types'
import { getLocale } from '$web-common/locale'

// Components
import {
  OnboardingContentLayout, //
} from '../components/onboarding_content_layout/onboarding_content_layout'
import {
  SelectImportTypeAction, //
} from './components/select_import_type_action/select_import_type_action'

// Styled Components
import {
  Column,
  Row,
  VerticalDivider,
  Text,
} from '$wallet/components/shared/style'

const softwareWalletIcons = [
  'brave-icon-release-color',
  'phantom-color',
  'metamask-color',
  'coinbase-color',
]
const hardwareWalletIcons = ['wallet-trezor', 'wallet-ledger']

export const OnboardingSelectImportType = () => {
  const history = useHistory()
  return (
    <OnboardingContentLayout>
      <Column
        width='100%'
        justifyContent='center'
        alignItems='center'
        gap='32px'
      >
        <Row
          maxWidth='420px'
          justifyContent='center'
          alignItems='center'
        >
          <Text
            variant='heading.display3'
            textColor='primary'
          >
            {getLocale(S.BRAVE_WALLET_IMPORT_WALLET_TYPE_TITLE)}
          </Text>
        </Row>
        <Column width='100%'>
          <SelectImportTypeAction
            title={getLocale(
              S.BRAVE_WALLET_IMPORT_WALLET_TYPE_HOT_WALLET_TITLE,
            )}
            description={getLocale(
              S.BRAVE_WALLET_IMPORT_WALLET_TYPE_HOT_WALLET_DESCRIPTION,
            )}
            icons={softwareWalletIcons}
            onClick={() => history.push(WalletRoutes.OnboardingImportTerms)}
          />
          <VerticalDivider margin='8px 0' />
          <SelectImportTypeAction
            title={getLocale(
              S.BRAVE_WALLET_IMPORT_WALLET_TYPE_HARDWARE_WALLET_TITLE,
            )}
            description={getLocale(
              S.BRAVE_WALLET_IMPORT_WALLET_TYPE_HARDWARE_WALLET_DESCRIPTION,
            )}
            icons={hardwareWalletIcons}
            onClick={() =>
              history.push(WalletRoutes.OnboardingHardwareWalletTerms)
            }
          />
        </Column>
      </Column>
    </OnboardingContentLayout>
  )
}
