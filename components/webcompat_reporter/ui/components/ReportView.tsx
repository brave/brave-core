/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components group
import {
  ModalLayout,
  ModalTitle,
  TextSection,
  InfoText,
  NonInteractiveURL,
  DisclaimerText,
  SideBySideButtons,
  PaddedButton,
  Input,
  TextArea,
  FieldCtr,
  InputLabel
} from './basic'

// Localization data
import { getLocale } from '../../../common/locale'

interface Props {
  siteUrl: string
  onSubmitReport: (details: string, contact: string) => void
  onClose: () => void
}

interface State {
  details: string
  contact: string
}

export default class ReportView extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { details: '', contact: '' }
  }

  render () {
    const {
      siteUrl,
      onSubmitReport,
      onClose
    } = this.props
    const { details, contact } = this.state
    return (
      <ModalLayout>
        <TextSection>
          <ModalTitle>{getLocale('reportModalTitle')}</ModalTitle>
        </TextSection>
        <InfoText>{getLocale('reportExplanation')}</InfoText>
        <NonInteractiveURL>{siteUrl}</NonInteractiveURL>
        <DisclaimerText>{getLocale('reportDisclaimer')}</DisclaimerText>
        <FieldCtr>
          <TextArea
            placeholder={getLocale('reportDetails')}
            onChange={(ev) => this.setState({ details: ev.target.value })}
            rows={7}
            maxLength={2000}
            value={details}
          />
        </FieldCtr>
        <FieldCtr>
          <InputLabel htmlFor='contact-info'>
            {getLocale('reportContactLabel')}
          </InputLabel>
          <Input
            placeholder={getLocale('reportContactPlaceholder')}
            onChange={(ev) => this.setState({ contact: ev.target.value })}
            type='text'
            maxLength={2000}
            value={contact}
            id='contact-info'
          />
        </FieldCtr>
        <SideBySideButtons>
          <PaddedButton
            text={getLocale('cancel')}
            level={'secondary'}
            type={'default'}
            size={'small'}
            onClick={onClose}
          />
          <PaddedButton
            text={getLocale('submit')}
            level={'primary'}
            type={'accent'}
            size={'small'}
            onClick={() => onSubmitReport(details, contact)}
          />
        </SideBySideButtons>
      </ModalLayout>
    )
  }
}
