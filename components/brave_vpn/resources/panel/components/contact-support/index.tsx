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
import * as S from './style'

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
    getLocale('braveVpnSupportSubjectOtherConnectionProblem')],
  ['noInternet',
    getLocale('braveVpnSupportSubjectNoInternet')],
  ['slowConnection',
    getLocale('braveVpnSupportSubjectSlowConnection')],
  ['websiteProblems',
    getLocale('braveVpnSupportSubjectWebsiteDoesntWork')],
  ['other',
    getLocale('braveVpnSupportSubjectOther')]
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
    getPanelBrowserAPI().panelHandler.openVpnUI('privacy')
  }

  // Setup any individual field error message.
  let emailAddressErrorMessage = ''
  if (showErrors) {
    if (formData.contactEmail && !emailAddressIsValid) {
      emailAddressErrorMessage = getLocale('braveVpnSupportEmailNotValid')
    } else {
      emailAddressErrorMessage = getLocale('braveVpnSupportFieldIsRequired')
    }
  }

  return (
    <S.Box>
      <S.PanelContent>
        <PanelHeader
          title={getLocale('braveVpnContactSupport')}
          buttonAriaLabel={getLocale('braveVpnSupportPanelBackButtonAriaLabel')}
          onClick={() => props.onCloseContactSupport()}
        />
        <S.TopContent>
          <S.Form onSubmit={(e) => e.preventDefault()}>
            <Input
              placeholder={getLocale('braveVpnSupportEmailInputPlaceholder')}
              showErrors={emailAddressErrorMessage.length !== 0}
              value={formData.contactEmail ?? ''}
              onChange={handleInputOnChange}
            >
              <S.StyledLabel>{getLocale('braveVpnSupportEmail')}</S.StyledLabel>
              <S.ErrorLabel slot='errors'>
                {emailAddressErrorMessage}
              </S.ErrorLabel>
            </Input>
            <Dropdown
              showErrors={showErrors && formData.problemSubject?.length === 0}
              value={SUBJECT_OPTIONS.get(problemSubject)}
              onChange={handleSelectOnChange}
            >
              <S.StyledDropdownPlaceholder slot="placeholder">
                {getLocale('braveVpnSupportSubjectNotSet')}
              </S.StyledDropdownPlaceholder>
              <S.StyledLabel slot="label">
                {getLocale('braveVpnSupportSubject')}
              </S.StyledLabel>
              {[...SUBJECT_OPTIONS.keys()].map((key) => {
                return (
                  <leo-option class='option' key={key} value={key}>
                    {SUBJECT_OPTIONS.get(key)}
                  </leo-option>
                )
              })}
              <S.ErrorLabel slot='errors'>
                {getLocale('braveVpnSupportSubjectNotSet')}
              </S.ErrorLabel>
            </Dropdown>
            <S.TextareaWrapper
              showError={showErrors && formData.problemBody?.length === 0}
            >
              <S.StyledLabel>{getLocale('braveVpnSupportBody')}</S.StyledLabel>
              <textarea
                placeholder={getLocale('braveVpnSupportDescriptionPlaceholder')}
                value={formData.problemBody}
                onChange={getOnChangeField('problemBody')}
              />
              {(showErrors && formData.problemBody?.length === 0) && (
                <S.ErrorLabel>
                  {getLocale('braveVpnSupportFieldIsRequired')}
                </S.ErrorLabel>
              )}
            </S.TextareaWrapper>
            <S.OptionalValues>
              <S.SectionDescription>
                {getLocale('braveVpnSupportOptionalHeader')}
              </S.SectionDescription>
              <S.Notes>
                <p>
                  {getLocale('braveVpnSupportOptionalNotes')}{' '}
                  <a
                    href='#'
                    onClick={handlePrivacyPolicyClick}
                  >
                    {getLocale('braveVpnSupportOptionalNotesPrivacyPolicy')}
                  </a>
                  .
                </p>
              </S.Notes>
              <S.OptionalValuesFields>
                <S.OptionalValueLabel
                  onClick={(e) => toggleOptionalValue('shareHostname')}
                >
                  <div className={'optionalValueTitle'}>
                    <span className={'optionalValueTitleKey'}>
                      {getLocale('braveVpnSupportOptionalVpnHostname')}
                    </span>{' '}
                    {supportData?.hostname}
                  </div>
                  <Toggle
                    checked={formData.shareHostname}
                    size='small'
                    onChange={getOnChangeToggle('shareHostname')}
                  />
                </S.OptionalValueLabel>
                <S.Divider />
                <S.OptionalValueLabel
                  onClick={(e) => toggleOptionalValue('shareAppVersion')}
                >
                  <div className={'optionalValueTitle'}>
                    <span className={'optionalValueTitleKey'}>
                      {getLocale('braveVpnSupportOptionalAppVersion')}
                    </span>{' '}
                    {supportData?.appVersion}
                  </div>
                  <Toggle
                    checked={formData.shareAppVersion}
                    size='small'
                    onChange={getOnChangeToggle('shareAppVersion')}
                  />
                </S.OptionalValueLabel>
                <S.Divider />
                <S.OptionalValueLabel
                  onClick={(e) => toggleOptionalValue('shareOsVersion')}
                >
                  <div className={'optionalValueTitle'}>
                    <span className={'optionalValueTitleKey'}>
                      {getLocale('braveVpnSupportOptionalOsVersion')}
                    </span>{' '}
                    {supportData?.osVersion}
                  </div>
                  <Toggle
                    checked={formData.shareOsVersion}
                    size='small'
                    onChange={getOnChangeToggle('shareOsVersion')}
                  />
                </S.OptionalValueLabel>
                <S.Divider />
                <S.OptionalValueLabel>
                  <div className={'optionalValueTitle'}>
                    <span className={'optionalValueTitleKey'}>
                      {getLocale('braveVpnSupportTimezone')}
                    </span>{' '}
                    {supportData?.timezone}
                  </div>
                </S.OptionalValueLabel>
              </S.OptionalValuesFields>
            </S.OptionalValues>
            <S.SupportNotes>
              {getLocale('braveVpnSupportNotes')}
            </S.SupportNotes>
            {isRemoteSubmissionError && (
              <S.ErrorLabel>
                {getLocale('braveVpnSupportTicketFailed')}
              </S.ErrorLabel>
            )}
            <S.StyledSubmitButton
              slot='actions'
              kind='filled'
              isLoading={isSubmitting}
              isDisabled={isSubmitting}
              onClick={handleSubmit}
            >
              {getLocale('braveVpnSupportSubmit')}
            </S.StyledSubmitButton>
          </S.Form>
        </S.TopContent>
      </S.PanelContent>
    </S.Box>
  )
}

export default ContactSupport
