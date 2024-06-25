// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Checkbox from '@brave/leo/react/checkbox'

// selectors
import { useSafeUISelector } from '../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../common/selectors'

// assets
import PageTermsGraphic from './assets/page_terms_graphic.svg'
import PanelTermsGraphic from './assets/panel_terms_graphic.svg'

// styles
import {
  TermsText,
  TermsDialog,
  Title,
  TermsLabel
} from './partners_consent_modal.style'
import { Row } from '../../../shared/style'
import Button from '@brave/leo/react/button'

interface PartnerConsentModalProps {
  isOpen: boolean
  onClose: () => void
  onContinue: () => void
  onCancel: () => void
}

export function PartnersConsentModal({
  isOpen,
  onClose,
  onContinue,
  onCancel
}: PartnerConsentModalProps) {
  // state
  const [termsAccepted, setTermsAccepted] = React.useState(false)

  // redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  return (
    <TermsDialog
      isOpen={isOpen}
      backdropClickCloses={false}
      escapeCloses={false}
      onClose={onClose}
      showClose
    >
      <Title slot='title'>Transactions Partner</Title>
      <Row justifyContent='center'>
        <img
          src={isPanel ? PanelTermsGraphic : PageTermsGraphic}
          alt='Terms Graphic'
        />
      </Row>
      <TermsText>
        Buying or selling crypto in Brave Wallet uses Meld.io — an
        on-ramp/off-ramp aggregator that provides a smooth experience and the
        best available pricing. Your information will be shared with Meld.io to
        complete the transaction.
      </TermsText>
      <Checkbox
        checked={termsAccepted}
        onChange={(e) => setTermsAccepted(e.checked)}
      >
        <TermsLabel>
          I have read and agree to the{' '}
          <a
            target='_blank'
            href='https://brave.com/terms-of-use/'
          >
            Terms of use
          </a>
        </TermsLabel>
      </Checkbox>
      <Row
        justifyContent='space-between'
        gap='16px'
        padding='24px 0 0'
      >
        <Button
          kind='outline'
          onClick={onCancel}
        >
          Cancel
        </Button>
        <Button
          isDisabled={!termsAccepted}
          onClick={onContinue}
        >
          Continue
        </Button>
      </Row>
    </TermsDialog>
  )
}
