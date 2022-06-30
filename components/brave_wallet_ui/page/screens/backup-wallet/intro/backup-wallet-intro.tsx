// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'

// components
import { Checkbox } from 'brave-ui'
import { NavButton } from '../../../../components/extension'

// style
import {
  StyledWrapper,
  Title,
  Description,
  TermsRow,
  SkipButton,
  PageIcon
} from './backup-wallet-intro.style'

export interface Props {
  onSubmit: () => void
  isBackupTermsAccepted: boolean
  isOnboarding: boolean
  onSubmitTerms: (key: string, selected: boolean) => void
  onCancel: () => void
  recoveryPhraseLength: number
}

export const BackupWalletIntroStep = ({
  onSubmit,
  isBackupTermsAccepted,
  isOnboarding,
  onSubmitTerms,
  onCancel,
  recoveryPhraseLength
}: Props) => {
  return (
    <StyledWrapper>
      <PageIcon />
      <Title>{getLocale('braveWalletBackupIntroTitle')}</Title>
      <Description>{getLocale('braveWalletBackupIntroDescription').replace('$1', recoveryPhraseLength.toString())}</Description>
      <TermsRow>
        <Checkbox value={{ backupTerms: isBackupTermsAccepted }} onChange={onSubmitTerms}>
          <div data-key='backupTerms'>{getLocale('braveWalletBackupIntroTerms')}</div>
        </Checkbox>
      </TermsRow>
      <NavButton disabled={!isBackupTermsAccepted} buttonType='primary' text={getLocale('braveWalletButtonContinue')} onSubmit={onSubmit} />
      <SkipButton onClick={onCancel}>{isOnboarding ? getLocale('braveWalletBackupButtonSkip') : getLocale('braveWalletBackupButtonCancel')}</SkipButton>
    </StyledWrapper>
  )
}

export default BackupWalletIntroStep
