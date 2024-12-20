// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Checkbox from '@brave/leo/react/checkbox'
import Button from '@brave/leo/react/button'

// Utils
import { getLocale, splitStringForTag } from '../../../../../common/locale'

// Selectors
import { useSafeUISelector } from '../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../common/selectors'

// Assets
import PageTermsGraphic from './assets/page_terms_graphic.svg'
import PanelTermsGraphic from './assets/panel_terms_graphic.svg'

// Styled Components
import {
  TermsText,
  TermsDialog,
  Title,
  TermsLabel,
  TermsButton
} from './partners_consent_modal.style'
import { Row } from '../../../shared/style'

interface PartnerConsentModalProps {
  isOpen: boolean
  onClose: () => void
  onContinue: () => void
}

export function PartnersConsentModal(
  props: Readonly<PartnerConsentModalProps>
) {
  const { isOpen, onClose, onContinue } = props

  // state
  const [termsAccepted, setTermsAccepted] = React.useState(false)

  // redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  const { beforeTag, duringTag } = splitStringForTag(
    getLocale('braveWalletMeldTermsOfUse'),
    1
  )

  const onClickTermsOfUse = () => {
    chrome.tabs.create(
      {
        url: 'https://www.meld.io/terms-of-use'
      },
      () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: ' + chrome.runtime.lastError.message
          )
        }
      }
    )
  }

  return (
    <TermsDialog
      isOpen={isOpen}
      backdropClickCloses={false}
      escapeCloses={false}
      onClose={onClose}
      showClose
    >
      <Title slot='title'>{getLocale('braveWalletTransactionsPartner')}</Title>
      <Row justifyContent='center'>
        <img
          src={isPanel ? PanelTermsGraphic : PageTermsGraphic}
          alt='Terms Graphic'
        />
      </Row>
      <TermsText>{getLocale('braveWalletTransactionPartnerConsent')}</TermsText>
      <Checkbox
        checked={termsAccepted}
        onChange={(e) => setTermsAccepted(e.checked)}
      >
        <TermsLabel>
          {beforeTag}
          <TermsButton onClick={onClickTermsOfUse}>{duringTag}</TermsButton>
        </TermsLabel>
      </Checkbox>
      <Row
        justifyContent='space-between'
        gap='16px'
        padding='24px 0 0'
      >
        <Button
          kind='outline'
          onClick={onClose}
        >
          {getLocale('braveWalletButtonCancel')}
        </Button>
        <Button
          isDisabled={!termsAccepted}
          onClick={onContinue}
        >
          {getLocale('braveWalletButtonContinue')}
        </Button>
      </Row>
    </TermsDialog>
  )
}
