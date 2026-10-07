// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// utils
import { getLocale } from '../../../../../common/locale'

// styles
import { ContinueButton } from '../../onboarding/onboarding.style'
import { SkipDialog, WarningText } from './explain-recovery-phrase.style'

interface SkipWarningDialogProps {
  isOpen: boolean
  onBack: () => void
  onSkip: () => void
}

export const SkipWarningDialog = ({
  isOpen,
  onBack,
  onSkip,
}: SkipWarningDialogProps) => {
  return (
    <SkipDialog
      isOpen={isOpen}
      onClose={onBack}
    >
      <WarningText>{getLocale(S.BRAVE_WALLET_SKIP_BACKUP_WARNING)}</WarningText>
      <div slot='actions'>
        <Button
          kind='plain-faint'
          onClick={onBack}
        >
          {getLocale(S.BRAVE_WALLET_GO_BACK)}
        </Button>
        <ContinueButton onClick={onSkip}>
          {getLocale(S.WALLET_SKIP_BACKUP)}
        </ContinueButton>
      </div>
    </SkipDialog>
  )
}
