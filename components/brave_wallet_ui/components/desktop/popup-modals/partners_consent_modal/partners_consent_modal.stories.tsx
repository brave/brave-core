// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import { PartnersConsentModal } from './partners_consent_modal'
import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'

export const _PartnersConsentModal = () => {
  const [isOpen, setIsOpen] = React.useState(true)

  return (
    <WalletPageStory>
      <PartnersConsentModal
        isOpen={isOpen}
        onClose={() => setIsOpen(false)}
        onContinue={() => setIsOpen(false)}
        onCancel={() => setIsOpen(false)}
      />
    </WalletPageStory>
  )
}

export default {
  component: _PartnersConsentModal,
  title: 'Partners Consent Modal'
}
