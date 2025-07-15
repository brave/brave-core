/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
} from '../../../components/basic'

// Fake data
import { getLocale } from '../fakeLocale'
import Button from '@brave/leo/react/button'

interface Props {
  siteUrl: string
  onSubmitReport: () => void
  onClose: () => void
}

export default class ReportView extends React.PureComponent<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    const {
      siteUrl,
      onSubmitReport,
      onClose
    } = this.props
    return (
      <ModalLayout>
        <TextSection>
          <ModalTitle>{getLocale('reportModalTitle')}</ModalTitle>
        </TextSection>
        <InfoText>{getLocale('reportExplanation')}</InfoText>
        <NonInteractiveURL>{siteUrl}</NonInteractiveURL>
        <DisclaimerText>{getLocale('reportDisclaimer')}</DisclaimerText>
        <SideBySideButtons>
          <Button
            kind="plain"
            size='small'
            onClick={onClose}
          >
            {getLocale('cancel')}
          </Button>
          <Button
            kind="filled"
            size='small'
            onClick={() => onSubmitReport()}
          >
            {getLocale('submit')}
          </Button>
        </SideBySideButtons>
      </ModalLayout>
    )
  }
}
