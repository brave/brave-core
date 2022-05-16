import * as React from 'react'
import Button from '$web-components/button'
import Select from '$web-components/select'
import TextInput, { Textarea } from '$web-components/input'
import Toggle from '$web-components/toggle'
import { getLocale } from '../../../../../common/locale'
import * as S from './style'
import getPanelBrowserAPI, * as BraveVPN from '../../api/panel_browser_api'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'

interface Props {
  onCloseContactSupport: React.MouseEventHandler<HTMLButtonElement>
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

type FormElement = HTMLInputElement | HTMLTextAreaElement | HTMLSelectElement
type BaseType = string | number | React.FormEvent<FormElement>

function ContactSupport (props: Props) {
  const [supportData, setSupportData] = React.useState<BraveVPN.SupportData>()
  const [formData, setFormData] = React.useState<ContactSupportState>(defaultSupportState)
  const [showErrors, setShowErrors] = React.useState(false)
  // Undefined for never sent, true for is sending, false for has completed
  const [isSubmitting, setIsSubmitting] = React.useState<boolean>()
  const [, setRemoteSubmissionError] = React.useState<string>()

  // Get possible values to submit
  React.useEffect(() => {
    getPanelBrowserAPI().serviceHandler.getSupportData()
      .then(setSupportData)
  }, [])

  function getOnChangeField<T extends BaseType = BaseType> (key: keyof ContactSupportInputFields) {
    return function (e: T) {
      console.log(setFormData)
      const value = (typeof e === 'string' || typeof e === 'number') ? e : e.currentTarget.value
      if (formData[key] === value) {
        return
      }
      setFormData(data => ({
        ...data,
        [key]: value
      }))
    }
  }

  function getOnChangeToggle (key: keyof ContactSupportToggleFields) {
    return function (isOn: boolean) {
      if (formData[key] === isOn) {
        return
      }
      setFormData(data => ({
        ...data,
        [key]: isOn
      }))
    }
  }

  const isValid = React.useMemo(() => {
    return !supportData ||
      !!formData.problemBody ||
      !!formData.contactEmail ||
      !!formData.problemSubject
  }, [formData, supportData])

  const handleSubmit = async () => {
    // Clear error about last submission
    setRemoteSubmissionError(undefined)
    // Handle submission when not valid, show user
    // which fields are required
    if (!isValid) {
      setShowErrors(true)
      return
    }
    // Handle is valid, submit data
    setIsSubmitting(true)
    const fullIssueBody = formData.problemBody + '\n' +
      (formData.shareOsVersion ? `OS: ${supportData?.osVersion}\n` : '') +
      (formData.shareAppVersion ? `App version: ${supportData?.appVersion}\n` : '') +
      (formData.shareHostname ? `Hostname: ${supportData?.hostname}\n` : '')

    // TODO: this will return a bool (success) and string (message)
    await getPanelBrowserAPI().serviceHandler.createSupportTicket(
      formData.contactEmail,
      formData.problemSubject,
      fullIssueBody
    )

    // TODO: handle error case, if any?
    setIsSubmitting(false)
  }

  const handlePrivacyPolicyClick = () => {
    getPanelBrowserAPI().panelHandler.openVpnUI('privacy')
  }

  return (
    <S.Box>
      <S.PanelContent>
        <S.PanelHeader>
          <S.BackButton
            type='button'
            onClick={props.onCloseContactSupport}
            aria-label='Close support form'
          >
            <i><CaratStrongLeftIcon /></i>
            <span>{getLocale('braveVpnContactSupport')}</span>
          </S.BackButton>
        </S.PanelHeader>

        <S.Form onSubmit={e => { e.preventDefault() }}>
          <TextInput
            label={getLocale('braveVpnSupportEmail')}
            isRequired={true}
            isErrorAlwaysShown={showErrors}
            value={formData.contactEmail ?? ''}
            onChange={getOnChangeField('contactEmail')}
          />
          <label>
            {getLocale('braveVpnSupportSubject')}
            <Select
              ariaLabel={getLocale('braveVpnSupportSubjectNotSet')}
              value={formData.problemSubject ?? ''}
              onChange={getOnChangeField('problemSubject')}
            >
              <option value="" disabled>{getLocale('braveVpnSupportSubjectNotSet')}</option>
              <option value="otherConnectionProblems">{getLocale('braveVpnSupportSubjectOtherConnectionProblem')}</option>
              <option value="noInternet">{getLocale('braveVpnSupportSubjectNoInternet')}</option>
              <option value="slowConnection">{getLocale('braveVpnSupportSubjectSlowConnection')}</option>
              <option value="websiteProblems">{getLocale('braveVpnSupportSubjectWebsiteDoesntWork')}</option>
              <option value="other">{getLocale('braveVpnSupportSubjectOther')}</option>
            </Select>
          </label>
          <Textarea
            value={formData.problemBody}
            label={getLocale('braveVpnSupportBody')}
            isRequired={true}
            onChange={getOnChangeField('problemBody')}
          />
          <S.OptionalValues>
            <S.SectionDescription>
              {getLocale('braveVpnSupportOptionalHeader')}
            </S.SectionDescription>
            <S.Notes>
              <p>
                {getLocale('braveVpnSupportOptionalNotes')}
                { ' ' }
                <a href="#" onClick={handlePrivacyPolicyClick}>
                  {getLocale('braveVpnSupportOptionalNotesPrivacyPolicy')}
                </a>.
              </p>
            </S.Notes>
            <S.OptionalValueLabel>
              <div className={'optionalValueTitle'}>
                <span className={'optionalValueTitleKey'}>
                  {getLocale('braveVpnSupportOptionalVpnHostname')}
                </span> {supportData?.hostname}
              </div>
              <Toggle
                isOn={formData.shareHostname}
                size={'sm'}
                onChange={getOnChangeToggle('shareHostname')}
              />
            </S.OptionalValueLabel>
            <S.OptionalValueLabel>
              <div className={'optionalValueTitle'}>
                <span className={'optionalValueTitleKey'}>
                  {getLocale('braveVpnSupportOptionalAppVersion')}
                </span> {supportData?.appVersion}
              </div>
              <Toggle
                isOn={formData.shareAppVersion}
                size={'sm'}
                onChange={getOnChangeToggle('shareAppVersion')}
              />
            </S.OptionalValueLabel>
            <S.OptionalValueLabel>
              <div className={'optionalValueTitle'}>
                <span className={'optionalValueTitleKey'}>
                  {getLocale('braveVpnSupportOptionalOsVersion')}
                </span> {supportData?.osVersion}
              </div>
              <Toggle
                isOn={formData.shareOsVersion}
                size={'sm'}
                onChange={getOnChangeToggle('shareOsVersion')}
              />
            </S.OptionalValueLabel>
          </S.OptionalValues>
          <S.Notes>
            <p>{getLocale('braveVpnSupportNotes')}</p>
          </S.Notes>
          <Button
            type={'submit'}
            isPrimary
            isCallToAction
            isLoading={isSubmitting}
            isDisabled={isSubmitting}
            onClick={handleSubmit}
          >
            {getLocale('braveVpnSupportSubmit')}
          </Button>
        </S.Form>

        {/* TODO(petemill): show an error if remoteSubmissionError has value */}
      </S.PanelContent>
    </S.Box>
  )
}

export default ContactSupport
