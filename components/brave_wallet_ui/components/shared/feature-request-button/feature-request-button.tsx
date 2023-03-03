// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import { Button, ButtonText, IdeaButtonIcon } from './feature-request-button.style'

const featureRequestUrl = 'https://community.brave.com/tags/c/wallet/131/feature-request'

export const FeatureRequestButton = () => {
  const onClickFeatureRequestButton = React.useCallback(() => {
    chrome.tabs.create({ url: featureRequestUrl }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }, [])

  return (
    <Button onClick={onClickFeatureRequestButton}>
      <IdeaButtonIcon />
      <ButtonText>{getLocale('braveWalletRequestFeatureButtonText')}</ButtonText>
    </Button>
  )
}
