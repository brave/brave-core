// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import isValidEmailAddress from '$web-common/isValidEmailAddress'
import { getLocale } from '$web-common/locale'
import Dropdown from '@brave/leo/react/dropdown'
import Input from '@brave/leo/react/input'
import Toggle from '@brave/leo/react/toggle'
import { PanelHeader } from '../select-region-list'
import getPanelBrowserAPI, * as BraveVPN from '../../api/panel_browser_api'
import * as Styles from './style'

interface Props {
  onCloseContactSupport: () => void
}

interface ContactSupportInputFields {
  contactEmail: string
  problemSubject: string
  problemBody: string
}

interface ContactSupportToggleFields {
  shareHostname: boolean
  shareAppVersion: boolean
  shareOsVersion: boolean
}

type ContactSupportState = ContactSupportInputFields &
  ContactSupportToggleFields

const defaultSupportState: ContactSupportState = {
  contactEmail: '',
  problemSubject: '',
  problemBody: '',
  shareHostname: true,
  shareAppVersion: true,
  shareOsVersion: true
}

const SUBJECT_OPTIONS = new Map([
  ['otherConnectionProblems',
    getLocale(S.BRAVE_VPN_SUPPORT_SUBJECT_OTHER_CONNECTION_PROBLEM)],
  ['noInternet',
    getLocale(S.BRAVE_VPN_SUPPORT_SUBJECT_NO_INTERNET)],
  ['slowConnection',
    getLocale(S.BRAVE_VPN_SUPPORT_SUBJECT_SLOW_CONNECTION)],
  ['websiteProblems',
    getLocale(S.BRAVE_VPN_SUPPORT_SUBJECT_WEBSITE_DOESNT_WORK)],
  ['other',
    getLocale(S.BRAVE_VPN_SUPPORT_SUBJECT_OTHER)]
])

type FormElement = HTMLInputElement | HTMLTextAreaElement | HTMLSelectElement
type BaseType = string | number | React.FormEvent<FormElement>

function ContactSupport(props: Props) {
  const [supportData, setSupportData] = React.useState<BraveVPN.SupportData>()
  const [formData, setFormData] =
    React.useState<ContactSupportState>(defaultSupportState)
  const [showErrors, setShowErrors] = React.useState(false)
  // Undefined for never sent, true for is sending, false for has completed
  const [isSubmitting, setIsSubmitting] = React.useState<boolean>()
  const [isRemoteSubmissionError, setRemoteSubmissionError] =
    React.useState<boolean>(false)
  const [problemSubject, setProblemSubject] = React.useState('')

  // Get possible values to submit
  React.useEffect(() => {
    getPanelBrowserAPI().serviceHandler.getSupportData().then(setSupportData)
  }, [])

  function getOnChangeField<T extends BaseType = BaseType>(
    key: keyof ContactSupportInputFields
  ) {
    return function (e: T) {
      const value =
        typeof e === 'string' || typeof e === 'number'
          ? e
          : e.currentTarget.value
      if (formData[key] === value) {
        return
      }
      setFormData((data) => ({
        ...data,
        [key]: value
      }))
    }
  }

  function getOnChangeToggle(key: keyof ContactSupportToggleFields) {
    return ({ checked }: { checked: boolean }) => {
      if (formData[key] === checked) {
        return
      }
      setFormData((data) => ({
        ...data,
        [key]: checked
      }))
    }
  }

  function toggleOptionalValue(key: keyof ContactSupportToggleFields) {
    setFormData((data) => ({
      ...data,
      [key]: !formData[key]
    }))
  }

  const emailAddressIsValid = React.useMemo(
    () => isValidEmailAddress(formData.contactEmail),
    [formData.contactEmail]
  )

  const isValid = React.useMemo(() => {
    return (
      !!formData.problemBody &&
      !!formData.contactEmail &&
      emailAddressIsValid &&
      !!formData.problemSubject
    )
  }, [formData, emailAddressIsValid])

  // Reset error states when data changes
  React.useEffect(() => {
    setShowErrors(false)
  }, [formData])

  const handleSubmit = async () => {
    // Clear error about last submission
    setRemoteSubmissionError(false)
    // Handle submission when not valid, show user
    // which fields are required
    if (!isValid) {
      setShowErrors(true)
      return
    }
    // Handle is valid, submit data
    setIsSubmitting(true)
    const fullIssueBody =
      `Message: \n${formData.problemBody}\n\n` +
      (formData.shareOsVersion ? `OS: ${supportData?.osVersion}\n` : '') +
      (formData.shareAppVersion
        ? `App version: ${supportData?.appVersion}\n`
        : '') +
      (formData.shareHostname ? `Hostname: ${supportData?.hostname}\n` : '')

    const { success } =
      await getPanelBrowserAPI().serviceHandler.createSupportTicket(
        formData.contactEmail,
        formData.problemSubject,
        fullIssueBody
      )

    setIsSubmitting(false)
    setRemoteSubmissionError(!success)

    if (success) {
      props.onCloseContactSupport()
    }
  }

  const handleSelectOnChange = ({ value }: { value: string }) => {
    setProblemSubject(value)
    setFormData((data) => ({
      ...data,
      'problemSubject': value
    }))
  }

  const handleInputOnChange = ({ value }: { value: string }) => {
    setFormData((data) => ({
      ...data,
      'contactEmail': value
    }))

  }

  const handlePrivacyPolicyClick = () => {
    getPanelBrowserAPI().panelHandler.openVpnUI(BraveVPN.ManageURLType.PRIVACY)
  }

  // Setup any individual field error message.
  let emailAddressErrorMessage = ''
  if (showErrors) {
    if (formData.contactEmail && !emailAddressIsValid) {
      emailAddressErrorMessage = getLocale(S.BRAVE_VPN_SUPPORT_EMAIL_NOT_VALID)
    } else {
      emailAddressErrorMessage = getLocale(S.BRAVE_VPN_SUPPORT_FIELD_IS_REQUIRED)
    }
  }

  return (
    <Styles.Box>
      <Styles.PanelContent>
        <PanelHeader
          title={getLocale(S.BRAVE_VPN_CONTACT_SUPPORT)}
          buttonAriaLabel={getLocale(S.BRAVE_VPN_SUPPORT_PANEL_BACK_BUTTON_ARIA_LABEL)}
          onClick={() => props.onCloseContactSupport()}
        />
        <Styles.TopContent>
          <Styles.Form onSubmit={(e) => e.preventDefault()}>
            <Input
              placeholder={getLocale(S.BRAVE_VPN_SUPPORT_EMAIL_PLACEHOLDER)}
              showErrors={emailAddressErrorMessage.length !== 0}
              value={formData.contactEmail ?? ''}
              onChange={handleInputOnChange}
            >
              <Styles.StyledLabel>{getLocale(S.BRAVE_VPN_SUPPORT_EMAIL)}</Styles.StyledLabel>
              <Styles.ErrorLabel slot='errors'>
                {emailAddressErrorMessage}
              </Styles.ErrorLabel>
            </Input>
            <Dropdown
              showErrors={showErrors && formData.problemSubject?.length === 0}
              value={SUBJECT_OPTIONS.get(problemSubject)}
              onChange={handleSelectOnChange}
            >
              <Styles.StyledDropdownPlaceholder slot="placeholder">
                {getLocale(S.BRAVE_VPN_SUPPORT_SUBJECT_NOTSET)}
              </Styles.StyledDropdownPlaceholder>
              <Styles.StyledLabel slot="label">
                {getLocale(S.BRAVE_VPN_SUPPORT_SUBJECT)}
              </Styles.StyledLabel>
              {[...SUBJECT_OPTIONS.keys()].map((key) => {
                return (
                  <leo-option class='option' key={key} value={key}>
                    {SUBJECT_OPTIONS.get(key)}
                  </leo-option>
                )
              })}
              <Styles.ErrorLabel slot='errors'>
                {getLocale(S.BRAVE_VPN_SUPPORT_SUBJECT_NOTSET)}
              </Styles.ErrorLabel>
            </Dropdown>
            <Styles.TextareaWrapper
              showError={showErrors && formData.problemBody?.length === 0}
            >
              <Styles.StyledLabel>{getLocale(S.BRAVE_VPN_SUPPORT_BODY)}</Styles.StyledLabel>
              <textarea
                placeholder={getLocale(S.BRAVE_VPN_SUPPORT_BODY_PLACEHOLDER)}
                value={formData.problemBody}
                onChange={getOnChangeField('problemBody')}
              />
              {(showErrors && formData.problemBody?.length === 0) && (
                <Styles.ErrorLabel>
                  {getLocale(S.BRAVE_VPN_SUPPORT_FIELD_IS_REQUIRED)}
                </Styles.ErrorLabel>
              )}
            </Styles.TextareaWrapper>
            <Styles.OptionalValues>
              <Styles.SectionDescription>
                {getLocale(S.BRAVE_VPN_SUPPORT_OPTIONAL_HEADER)}
              </Styles.SectionDescription>
              <Styles.Notes>
                <p>
                  {getLocale(S.BRAVE_VPN_SUPPORT_OPTIONAL_NOTES)}{' '}
                  <a
                    href='#'
                    onClick={handlePrivacyPolicyClick}
                  >
                    {getLocale(S.BRAVE_VPN_SUPPORT_OPTIONAL_NOTES_PRIVACY_POLICY)}
                  </a>
                  .
                </p>
              </Styles.Notes>
              <Styles.OptionalValuesFields>
                <Styles.OptionalValueLabel
                  onClick={(e) => toggleOptionalValue('shareHostname')}
                >
                  <div className={'optionalValueTitle'}>
                    <span className={'optionalValueTitleKey'}>
                      {getLocale(S.BRAVE_VPN_SUPPORT_OPTIONAL_VPN_HOSTNAME)}
                    </span>{' '}
                    {supportData?.hostname}
                  </div>
                  <Toggle
                    checked={formData.shareHostname}
                    size='small'
                    onChange={getOnChangeToggle('shareHostname')}
                  />
                </Styles.OptionalValueLabel>
                <Styles.Divider />
                <Styles.OptionalValueLabel
                  onClick={(e) => toggleOptionalValue('shareAppVersion')}
                >
                  <div className={'optionalValueTitle'}>
                    <span className={'optionalValueTitleKey'}>
                      {getLocale(S.BRAVE_VPN_SUPPORT_OPTIONAL_APP_VERSION)}
                    </span>{' '}
                    {supportData?.appVersion}
                  </div>
                  <Toggle
                    checked={formData.shareAppVersion}
                    size='small'
                    onChange={getOnChangeToggle('shareAppVersion')}
                  />
                </Styles.OptionalValueLabel>
                <Styles.Divider />
                <Styles.OptionalValueLabel
                  onClick={(e) => toggleOptionalValue('shareOsVersion')}
                >
                  <div className={'optionalValueTitle'}>
                    <span className={'optionalValueTitleKey'}>
                      {getLocale(S.BRAVE_VPN_SUPPORT_OPTIONAL_OS_VERSION)}
                    </span>{' '}
                    {supportData?.osVersion}
                  </div>
                  <Toggle
                    checked={formData.shareOsVersion}
                    size='small'
                    onChange={getOnChangeToggle('shareOsVersion')}
                  />
                </Styles.OptionalValueLabel>
                <Styles.Divider />
                <Styles.OptionalValueLabel>
                  <div className={'optionalValueTitle'}>
                    <span className={'optionalValueTitleKey'}>
                      {getLocale(S.BRAVE_VPN_SUPPORT_TIMEZONE)}
                    </span>{' '}
                    {supportData?.timezone}
                  </div>
                </Styles.OptionalValueLabel>
              </Styles.OptionalValuesFields>
            </Styles.OptionalValues>
            <Styles.SupportNotes>
              {getLocale(S.BRAVE_VPN_SUPPORT_NOTES)}
            </Styles.SupportNotes>
            {isRemoteSubmissionError && (
              <Styles.ErrorLabel>
                {getLocale(S.BRAVE_VPN_SUPPORT_TICKET_FAILED)}
              </Styles.ErrorLabel>
            )}
            <Styles.StyledSubmitButton
              slot='actions'
              kind='filled'
              isLoading={isSubmitting}
              isDisabled={isSubmitting}
              onClick={handleSubmit}
            >
              {getLocale(S.BRAVE_VPN_SUPPORT_SUBMIT)}
            </Styles.StyledSubmitButton>
          </Styles.Form>
        </Styles.TopContent>
      </Styles.PanelContent>
    </Styles.Box>
  )
}

export default ContactSupport
