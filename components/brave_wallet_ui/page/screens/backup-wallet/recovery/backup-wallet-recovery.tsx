// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'

// components
import { Checkbox } from 'brave-ui'
import { Tooltip } from '../../../../components/shared'
import { NavButton } from '../../../../components/extension'

// style
import {
  StyledWrapper,
  Title,
  Description,
  TermsRow,
  CopyButton,
  WarningBox,
  WarningText,
  DisclaimerText,
  DisclaimerColumn,
  AlertIcon,
  RecoveryBubble,
  RecoveryBubbleText,
  RecoveryPhraseContainer,
  BigCheckMark
} from './backup-wallet-recovery.style'

export interface Props {
  onSubmit: () => void
  isRecoveryTermsAccepted: boolean
  onSubmitTerms: (key: string, selected: boolean) => void
  recoverPhrase: string[]
  isRecoverPhraseCopied: boolean
  onCopy: () => void
}

export const BackupWalletRecoveryStep = ({
  onSubmit,
  isRecoveryTermsAccepted,
  onSubmitTerms,
  recoverPhrase,
  isRecoverPhraseCopied,
  onCopy
}: Props) => {
  return (
    <>
      <StyledWrapper>
        <Title>{getLocale('braveWalletRecoveryTitle')}</Title>
        <Description>{getLocale('braveWalletRecoveryDescription')}</Description>
        <WarningBox>
          <AlertIcon />
          <DisclaimerColumn>
            <DisclaimerText><WarningText>{getLocale('braveWalletRecoveryWarning1')} </WarningText>{getLocale('braveWalletRecoveryWarning2')}</DisclaimerText>
            <DisclaimerText>{getLocale('braveWalletRecoveryWarning3')}</DisclaimerText>
          </DisclaimerColumn>
        </WarningBox>
        <RecoveryPhraseContainer>
          {recoverPhrase.map((word, index) =>
            <RecoveryBubble key={index}>
              <RecoveryBubbleText>{index + 1}. {word}</RecoveryBubbleText>
            </RecoveryBubble>
          )}
        </RecoveryPhraseContainer>

        <Tooltip
          text={
            getLocale('braveWalletToolTipCopyToClipboard')
          }
          isVisible={!isRecoverPhraseCopied}
          verticalPosition='below'
          pointerPosition='center'
        >
          <CopyButton onClick={onCopy}>
            {isRecoverPhraseCopied && <BigCheckMark />}
            {isRecoverPhraseCopied ? getLocale('braveWalletButtonCopied') : getLocale('braveWalletButtonCopy')}
          </CopyButton>
        </Tooltip>

        <TermsRow>
          <Checkbox value={{ backedUp: isRecoveryTermsAccepted }} onChange={onSubmitTerms}>
            <div data-key='backedUp'>{getLocale('braveWalletRecoveryTerms')}</div>
          </Checkbox>
        </TermsRow>
        <NavButton disabled={!isRecoveryTermsAccepted} buttonType='primary' text={getLocale('braveWalletButtonContinue')} onSubmit={onSubmit} />
      </StyledWrapper>
    </>
  )
}

export default BackupWalletRecoveryStep
