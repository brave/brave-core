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
  InputLabel,
  ScreenshotLink,
  Dropdown
} from './basic'

// Localization data
import { getLocale } from '../../../common/locale'
import {
  captureScreenshot,
  clearScreenshot,
  getCapturedScreenshot,
} from '../browser_proxy'

interface Props {
  siteUrl: string
  contactInfo: string
  contactInfoSaveFlag: boolean
  isErrorPage: boolean
  isHttpPage: boolean
  isLocalPage: boolean
  components: string[]
  onSubmitReport: (category: string, details: string, contact: string,
                   attachScreenshot: boolean) => void
  onClose: () => void
}

interface State {
  category: string
  details: string
  contact: string
  attachScreenshot: boolean
  screenshotObjectUrl: string | null
  hideViewScreenshotLink: boolean
}

const WEBCOMPAT_INFO_WIKI_URL = 'https://github.com/brave/brave-browser/wiki/Web-compatibility-reports'

export default class ReportView extends React.PureComponent<Props, State> {
  private handleKeyPress: (e: KeyboardEvent) => void;

  constructor (props: Props) {
    super(props)
    this.state = {
      category: '',
      details: '',
      contact: props.contactInfo,
      attachScreenshot: false,
      screenshotObjectUrl: null,
      hideViewScreenshotLink: false
    }
    this.handleKeyPress = this._handleKeyPress.bind(this);
  }

  private _handleKeyPress(e: KeyboardEvent) {
    const { onClose } = this.props;
    if (e.key === 'Escape') {
      onClose();
    }
  }

  handleCategoryChange = async (ev: React.ChangeEvent<HTMLSelectElement>) => {
    this.setState({ category: ev.target.value })
  }

  handleContactInfoChange = async (ev: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ contact: ev.target.value })
  }

  handleScreenshotChange = async (ev: React.ChangeEvent<HTMLInputElement>) => {
    if (this.state.screenshotObjectUrl) {
      URL.revokeObjectURL(this.state.screenshotObjectUrl)
    }
    if (!ev.target.checked) {
      this.setState({ attachScreenshot: false, screenshotObjectUrl: null })
      clearScreenshot()
      return
    }

    try {
      await captureScreenshot()
    } catch {
      this.setState({
        attachScreenshot: true,
        screenshotObjectUrl: null,
        hideViewScreenshotLink: true
      })
        clearScreenshot()
        return
    }
    this.setState({ attachScreenshot: true, screenshotObjectUrl: null })
  }

  // the element for the onClick is an <a>. generate an ev typescript react type for ev
  handleViewScreenshot = async (ev: React.MouseEvent<HTMLAnchorElement, MouseEvent>) => {
    ev.preventDefault()
    if (this.state.screenshotObjectUrl) {
      window.open(this.state.screenshotObjectUrl, '_blank', 'noopener')
      return
    }
    const encodedScreenshot = await getCapturedScreenshot()
    const decodedScreenshot = Buffer.from(encodedScreenshot, 'base64')
    const blob = new Blob([decodedScreenshot], { type: 'image/png' })
    const screenshotObjectUrl = URL.createObjectURL(blob)

    this.setState({ screenshotObjectUrl })

    window.open(screenshotObjectUrl, '_blank', 'noopener')
  }

  componentDidMount() {
    document.addEventListener('keydown', this.handleKeyPress);
  }

  componentWillUnmount() {
    document.removeEventListener('keydown', this.handleKeyPress);
  }

  /**
   * Only reports with a selected category (or in the case of `Other`, a filled
   * in description) can be submitted.
   */
  canSubmitReport() {
    if (this.state.category === '') {
      return false;
    }
    if (this.state.category === 'other' && this.state.details === '') {
      return false;
    }
    return true;
  }

  render () {
    const {
      siteUrl,
      contactInfoSaveFlag,
      isErrorPage,
      isHttpPage,
      isLocalPage,
      onSubmitReport,
      onClose,
      components
    } = this.props
    const { category, details, contact, attachScreenshot } = this.state

    const isIneligiblePage = !isHttpPage || isLocalPage || isErrorPage

    let infoTextKey = 'reportExplanation'
    if (!isHttpPage) {
      infoTextKey = 'reportNonHttpExplanation'
    } else if (isLocalPage) {
      infoTextKey = 'reportLocalExplanation'
    } else if (isErrorPage) {
      infoTextKey = 'reportErrorPageExplanation'
    }

    /** Apply `hidden` CSS class unless the given component ID is present */
    const hideUnlessHas = (componentId: string) => {
      return components.includes(componentId) ? '' : 'hidden'
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
            <InputLabel htmlFor='issue-category'>
              {getLocale('issueCategoryLabel')}
            </InputLabel>
            <Dropdown
              onChange={this.handleCategoryChange}
              value={category}
              id='issue-category'
              required={true}
            >
              <option value='' disabled hidden>
                {getLocale('issueCategoryPlaceholder')}
              </option>
              <option value='ads'>{getLocale('issueCategoryAds')}</option>
              <option value='browser not supported'>
                {getLocale('issueCategoryBrowserNotSupported')}
              </option>
              <option value='blank'>{getLocale('issueCategoryBlank')}</option>
              <option value='scroll'>{getLocale('issueCategoryScroll')}</option>
              <option value='form'>{getLocale('issueCategoryForm')}</option>
              <option
                value='cookie notice'
                className={hideUnlessHas('cdbbhgbmjhfnhnmgeddbliobbofkgdhe')}
              >
                {getLocale('issueCategoryCookie')}
              </option>
              <option value='anti adblock'>
                {getLocale('issueCategoryAntiadblock')}
              </option>
              <option value='tracking'>
                {getLocale('issueCategoryTracking')}
              </option>
              <option
                value='newsletter'
                className={hideUnlessHas('kdddfellohomdnfkdhombbddhojklibj')}
              >
                {getLocale('issueCategoryNewsletter')}
              </option>
              <option
                value='social'
                className={hideUnlessHas('nbkknaieglghmocpollinelcggiehfco')}
              >
                {getLocale('issueCategorySocial')}
              </option>
              <option
                value='chat'
                className={hideUnlessHas('cjoooeeofnfjohnalnghhmdlalopplja')}
              >
                {getLocale('issueCategoryChat')}
              </option>
              <option value='other'>{getLocale('issueCategoryOther')}</option>
            </Dropdown>
            <FieldCtr>
              <TextArea
                placeholder={getLocale('reportDetails') +
                  (category !== 'other' ? ` ${getLocale('optionalField')}` : '')
                }
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
                onChange={this.handleContactInfoChange}
                type='text'
                maxLength={2000}
                value={contact}
                id='contact-info'
              />
            </FieldCtr>
            {contactInfoSaveFlag &&
              <FieldCtr>
                <InputLabel>
                  {getLocale('reportContactPopupInfoLabel')}
                </InputLabel>
              </FieldCtr>
            }
            <FieldCtr>
              <Checkbox
                onChange={this.handleScreenshotChange}
                type='checkbox'
                checked={attachScreenshot}
                id='attach-screenshot'
              />
              <CheckboxLabel htmlFor='attach-screenshot'>
                {getLocale('attachScreenshotLabel')}
              </CheckboxLabel>
            </FieldCtr>
            {!!this.state.attachScreenshot &&
              !this.state.hideViewScreenshotLink &&
              <ScreenshotLink onClick={this.handleViewScreenshot}>
                {getLocale('viewScreenshotLabel')}
              </ScreenshotLink>
            }
            <DisclaimerText>
              {getLocale('reportDisclaimer')}
              &nbsp;
              <a href={WEBCOMPAT_INFO_WIKI_URL} target="_blank">
                {getLocale('reportInfoLink')}
              </a>
            </DisclaimerText>
          </>
        }
        <SideBySideButtons>
          {!isIneligiblePage ?
          <>
            <PaddedButton
              kind="plain-faint"
              size="small"
              onClick={onClose}
            >
              {getLocale('cancel')}
            </PaddedButton>
            <PaddedButton
              kind="filled"
              size='small'
              isDisabled={!this.canSubmitReport()}
              onClick={() => onSubmitReport(category, details, contact,
                                            attachScreenshot)}
            >
              {getLocale('submit')}
            </PaddedButton>
          </>
        :
          <PaddedButton
            kind="filled"
            size="small"
            onClick={onClose}
          >
            {getLocale('close')}
          </PaddedButton>
        }
        </SideBySideButtons>
      </ModalLayout>
    )
  }
}
