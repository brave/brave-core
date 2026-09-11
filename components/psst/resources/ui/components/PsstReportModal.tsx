/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'

import { color, font } from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

import Flex from '$web-common/Flex'
import { formatLocale, getLocale } from '$web-common/locale'
import { Container, PsstDlgButton, RightAlignedItem } from './basic/structure'

import '../strings'

// Styled components
const ModalTitleRow = styled.div`
  display: flex;
  align-items: center;
  gap: ${leo.spacing.m};
`

const ModalTitle = styled.div`
  margin: ${leo.spacing['2Xl']} 0;
  font: ${font.heading.h4};
  font-size: ${leo.typography.heading.h4.fontSize};
  line-height: ${leo.typography.heading.h4.lineHeight};
  color: ${color.text.secondary};
`

const ModalTitleBody = styled.div`
  font: ${leo.font.default.regular};
  color: ${leo.color.text.secondary};
  letter-spacing: ${leo.typography.letterSpacing.small};
  margin-bottom: ${leo.spacing['2Xl']};

  a {
    color: ${color.text.interactive};
    text-decoration: underline;
  }
`

const ReportCard = styled.div`
  border-radius: ${leo.radius.m};
  border: ${color.divider.subtle} 1px solid;
  margin-bottom: ${leo.spacing.xl};
`

const ReportCardSection = styled.div`
  padding: ${leo.spacing.l} ${leo.spacing.xl};

  & + & {
    border-top: ${color.divider.subtle} 1px solid;
  }
`

const ReportCardLabel = styled.div`
  font: ${font.default.semibold};
  color: ${color.text.secondary};
  margin-bottom: ${leo.spacing.s};
`

const ReportCardValue = styled.div`
  font: ${font.default.regular};
  color: ${color.text.secondary};
`

const FailedStepRow = styled(ReportCardValue)`
  white-space: pre-wrap;
`

const SentBanner = styled.div`
  border-radius: ${leo.radius.m};
  padding: ${leo.spacing.l} ${leo.spacing.xl};
  margin-bottom: ${leo.spacing.xl};
  background: ${color.systemfeedback.successBackground};
  color: ${color.systemfeedback.successText};
  font: ${font.default.regular};
`

const SentButton = styled(PsstDlgButton)`
  --leo-color-button-background: ${leo.color.button.successBackground};
  --leo-color-schemes-on-primary: ${leo.color.button.successText}
`

export interface Props {
  siteName: string
  failedSteps: string[]
  isSending: boolean
  isSent: boolean
  onBack: () => void
  onClose: () => void
  onSendReport: () => void
}

export const PsstReportModal: React.FC<Props> = ({
  siteName,
  failedSteps,
  isSending,
  isSent,
  onBack,
  onClose,
  onSendReport,
}) => {
  return (
    <Container>
      <Flex
        direction='row'
        justify='space-between'
        align='center'
      >
        <ModalTitleRow>
          <Button
            fab
            kind='plain-faint'
            onClick={onBack}
          >
            <Icon name='arrow-left' />
          </Button>
          <ModalTitle>{getLocale(S.PSST_REPORT_DIALOG_TITLE)}</ModalTitle>
        </ModalTitleRow>
        <RightAlignedItem>
          <Button
            fab
            kind='plain-faint'
            onClick={onClose}
          >
            <Icon name='close-circle' />
          </Button>
        </RightAlignedItem>
      </Flex>
      <ModalTitleBody>
        {formatLocale(S.PSST_REPORT_DIALOG_BODY, {
          $1: (content) => (
            <a
              href={getLocale(S.PSST_REPORT_DIALOG_BODY_LEARN_MORE_LINK)}
              target='_blank'
              rel='noopener noreferrer'
            >
              {content}
            </a>
          ),
        })}
      </ModalTitleBody>
      <ReportCard>
        <ReportCardSection>
          <ReportCardLabel>
            {getLocale(S.PSST_REPORT_DIALOG_CURRENT_URL_LABEL)}
          </ReportCardLabel>
          <ReportCardValue>{siteName}</ReportCardValue>
        </ReportCardSection>
        <ReportCardSection>
          <ReportCardLabel>
            {getLocale(S.PSST_REPORT_DIALOG_FAILED_STEPS_LABEL)}
          </ReportCardLabel>
          {failedSteps.map((step, index) => (
            <FailedStepRow key={index}>- {step}</FailedStepRow>
          ))}
        </ReportCardSection>
      </ReportCard>
      {isSent && (
        <SentBanner>{getLocale(S.PSST_REPORT_DIALOG_SENT_MESSAGE)}</SentBanner>
      )}
      <RightAlignedItem>
        <PsstDlgButton
          kind='outline'
          size='medium'
          isDisabled={isSent || isSending}
          onClick={onClose}
        >
          {getLocale(S.PSST_COMPLETE_CONSENT_DIALOG_CANCEL)}
        </PsstDlgButton>
        {isSent ? (
          <SentButton
            kind='filled'
            size='medium'
            onClick={onClose}
          >
            <Icon
              slot='icon-before'
              name='check-circle-outline'
            />
            {getLocale(S.PSST_REPORT_DIALOG_SENT_BUTTON)}
          </SentButton>
        ) : (
          <PsstDlgButton
            kind='filled'
            size='medium'
            isDisabled={isSending}
            isLoading={isSending}
            onClick={onSendReport}
          >
            {getLocale(S.PSST_REPORT_DIALOG_SEND_BUTTON)}
          </PsstDlgButton>
        )}
      </RightAlignedItem>
    </Container>
  )
}

export default React.memo(PsstReportModal)
