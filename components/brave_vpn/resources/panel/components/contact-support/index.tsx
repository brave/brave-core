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

    await getPanelBrowserAPI().serviceHandler.createSupportTicket(
      formData.contactEmail,
      formData.problemSubject,
      fullIssueBody
    )

    // TODO: handle error case, if any?
    setIsSubmitting(false)
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
            label={'Your email address'}
            isRequired={true}
            isErrorAlwaysShown={showErrors}
            value={formData.contactEmail ?? ''}
            onChange={getOnChangeField('contactEmail')}
          />
          <label>
            Subject
            <Select
              ariaLabel={'Please choose a reason'}
              value={formData.problemSubject ?? ''}
              onChange={getOnChangeField('problemSubject')}
            >
              <option value="" disabled>Please choose a reason</option>
              <option value="cant-connect">Cannot connect to the VPN (Other error)</option>
              <option value="no-internet">No internet when connected</option>
              <option value="slow">Slow connection</option>
              <option value="website">Website doesn't work</option>
              <option value="other">Other</option>
            </Select>
          </label>
          <Textarea
            value={formData.problemBody}
            label={'Describe your issue'}
            isRequired={true}
            onChange={getOnChangeField('problemBody')}
          />
          <S.OptionalValues>
            <S.SectionDescription>
              Please select the information you're comfortable sharing with us
            </S.SectionDescription>
            <S.OptionalValueLabel>
              <div className={'optionalValueTitle'}>
                <span className={'optionalValueTitleKey'}>VPN hostname:</span> {supportData?.hostname}
              </div>
              <Toggle
                isOn={formData.shareHostname}
                size={'sm'}
                onChange={getOnChangeToggle('shareHostname')}
              />
            </S.OptionalValueLabel>
            <S.OptionalValueLabel>
              <div className={'optionalValueTitle'}>
                <span className={'optionalValueTitleKey'}>App version:</span> {supportData?.appVersion}
              </div>
              <Toggle
                isOn={formData.shareAppVersion}
                size={'sm'}
                onChange={getOnChangeToggle('shareAppVersion')}
              />
            </S.OptionalValueLabel>
            <S.OptionalValueLabel>
              <div className={'optionalValueTitle'}>
                <span className={'optionalValueTitleKey'}>OS version:</span> {supportData?.osVersion}
              </div>
              <Toggle
                isOn={formData.shareOsVersion}
                size={'sm'}
                onChange={getOnChangeToggle('shareOsVersion')}
              />
            </S.OptionalValueLabel>
          </S.OptionalValues>
          <S.Notes>
            <p>
              The more information you share with us the easier it will be for the support staff to help you resolve your issue.
            </p>
            <p>
              Support provided with the help of the Guardian team.
            </p>
          </S.Notes>
          <Button
            type={'submit'}
            isPrimary
            isCallToAction
            isLoading={isSubmitting}
            isDisabled={isSubmitting}
            onClick={handleSubmit}
          >
            Submit
          </Button>
        </S.Form>

        {/* TODO(petemill): show an error if remoteSubmissionError has value */}
      </S.PanelContent>
    </S.Box>
  )
}

export default ContactSupport
