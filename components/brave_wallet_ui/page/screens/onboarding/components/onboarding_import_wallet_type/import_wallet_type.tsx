// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// utils
import { WalletRoutes } from '../../../../../constants/types'
import { getLocale } from '../../../../../../common/locale'

// components
import {
  OnboardingContentLayout, //
} from '../onboarding_content_layout/content_layout'
import { ImportTypeAction } from './components/import_type_action'

// styles
import {
  Column, //
  VerticalDivider,
} from '../../../../../components/shared/style'

const softwareWalletIcons = [
  'brave-icon-release-color',
  'phantom-color',
  'metamask-color',
  'coinbase-color',
]
const hardwareWalletIcons = ['trezor-color', 'wallet-ledger']

export const OnboardingImportWalletType = () => {
  const history = useHistory()
  return (
    <OnboardingContentLayout
      title={getLocale(S.BRAVE_WALLET_IMPORT_WALLET_TYPE_TITLE)}
    >
      <Column
        width='100%'
        justifyContent='center'
        alignItems='flex-start'
        margin='92px 0 165px 0'
      >
        <ImportTypeAction
          title={getLocale(S.BRAVE_WALLET_IMPORT_WALLET_TYPE_HOT_WALLET_TITLE)}
          description={getLocale(
            S.BRAVE_WALLET_IMPORT_WALLET_TYPE_HOT_WALLET_DESCRIPTION,
          )}
          icons={softwareWalletIcons}
          onClick={() => history.push(WalletRoutes.OnboardingImportTerms)}
        />
        <VerticalDivider margin='8px 0' />
        <ImportTypeAction
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
    </OnboardingContentLayout>
  )
}
