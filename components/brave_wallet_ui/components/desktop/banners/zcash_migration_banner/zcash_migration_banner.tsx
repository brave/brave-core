// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../common/locale'
import { openTab } from '../../../../utils/routes-utils'

// Styled Components
import { Alert, Button } from './zcash_migration_banner.style'

export function ZCashMigrationBanner() {
  const onClickLearnMore = () => {
    openTab(
      'https://forum.zcashcommunity.com/t/ironwood-update-for-users/56721',
    )
  }
  return (
    <Alert type='warning'>
      {getLocale(S.BRAVE_WALLET_ZCASH_MIGRATION_BANNER_DESCRIPTION)}
      <Button
        kind='plain-faint'
        size='tiny'
        onClick={onClickLearnMore}
      >
        {getLocale(S.BRAVE_WALLET_LEARN_MORE)}
      </Button>
    </Alert>
  )
}
