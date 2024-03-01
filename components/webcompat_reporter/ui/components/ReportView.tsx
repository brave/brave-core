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
  PaddedButton,
  Input,
  Checkbox,
  CheckboxLabel,
  TextArea,
  FieldCtr,
  InputLabel
} from './basic'

// Localization data
import { getLocale } from '../../../common/locale'

interface Props {
  siteUrl: string
  isErrorPage: boolean
  isHttpPage: boolean
  isLocalPage: boolean
  onSubmitReport: (details: string, contact: string, attachScreenshot: boolean) => void
  onClose: () => void
}

interface State {
  details: string
  contact: string
  attachScreenshot: boolean
}

const WEBCOMPAT_INFO_WIKI_URL = 'https://github.com/brave/brave-browser/wiki/Web-compatibility-reports'

export default class ReportView extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { details: '', contact: '', attachScreenshot: false }
  }

  render () {
    const {
      siteUrl,
      isErrorPage,
      isHttpPage,
      isLocalPage,
      onSubmitReport,
      onClose
    } = this.props
    const { details, contact, attachScreenshot } = this.state

    const isIneligiblePage = !isHttpPage || isLocalPage || isErrorPage

    let infoTextKey = 'reportExplanation'
    if (!isHttpPage) {
      infoTextKey = 'reportInternalExplanation'
    } else if (isLocalPage) {
      infoTextKey = 'reportLocalExplanation'
    } else if (isErrorPage) {
      infoTextKey = 'reportErrorPageExplanation'
    }

    return (
      <ModalLayout>
        <TextSection>
          <ModalTitle>{getLocale('reportModalTitle')}</ModalTitle>
        </TextSection>
        <InfoText>
          {getLocale(infoTextKey)}
        </InfoText>
        {!isIneligiblePage &&
          <>
            <NonInteractiveURL>{siteUrl}</NonInteractiveURL>
            <DisclaimerText>
              {getLocale('reportDisclaimer')}
              &nbsp;
              <a href={WEBCOMPAT_INFO_WIKI_URL} target="_blank">
                {getLocale('reportInfoLink')}
              </a>
            </DisclaimerText>
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
            <FieldCtr>
              <Checkbox
                onChange={(ev) => this.setState({ attachScreenshot: ev.target.checked })}
                type='checkbox'
                checked={attachScreenshot}
                id='attach-screenshot'
              />
              <CheckboxLabel htmlFor='attach-screenshot'>
                {getLocale('attachScreenshotLabel')}
              </CheckboxLabel>
            </FieldCtr>
          </>
        }
        <SideBySideButtons>
          {!isIneligiblePage ?
          <>
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
              onClick={() => onSubmitReport(details, contact, attachScreenshot)}
            />
          </>
        :
          <PaddedButton
            text={getLocale('close')}
            level={'primary'}
            type={'accent'}
            size={'small'}
            onClick={onClose}
          />
        }
        </SideBySideButtons>
      </ModalLayout>
    )
  }
}
