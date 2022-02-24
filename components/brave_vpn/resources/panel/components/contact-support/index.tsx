import * as React from 'react'
import { Button } from 'brave-ui'

import { getLocale } from '../../../../../common/locale'
import * as S from './style'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'

interface Props {
  closeContactSupport: React.MouseEventHandler<HTMLButtonElement>
}

interface ContactSupportState {
  contactEmail: string
  problemSubject: string
  problemBody: string
  shareHostname: boolean
  shareAppVersion: boolean
  shareOsVersion: boolean
}

function ContactSupport (props: Props) {
  const [supportData, setSupportData] = React.useState('TODO: fill me in')

  React.useEffect(async () => {
    setSupportData(await getPanelBrowserAPI().getSupportData())
  })

  const [formData, setFormData] = React.useState<ContactSupportState>({
    contactEmail: '',
    problemSubject: '',
    problemBody: '',
    shareHostname: true,
    shareAppVersion: true,
    shareOsVersion: true
  })

  const isValid = React.useMemo(() => {
    return !!formData?.problemBody
  }, [formData])

  const handleSubmit = () => {
    // Build submit data
  }

  const onChangeBody = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
    setFormData({
      ...formData,
      problemBody: event.currentTarget.value
    })
    console.log('changed')
  }

  return (
    <S.Box>
      <S.PanelContent>
        <S.PanelHeader>
          <S.BackButton
            type='button'
            onClick={props.closeContactSupport}
            aria-label='Close support form'
          >
            <i><CaratStrongLeftIcon /></i>
            <span>{getLocale('braveVpnContactSupport')}</span>
          </S.BackButton>
        </S.PanelHeader>
        <S.List>
          <li>
            Your email address
          </li>
          <li>
            Subject
            <select value={formData?.problemSubject || ''} name="issue" id="contact-support-issue">
              <option value="" disabled>Please choose a reason</option>
              <option value="cant-connect">Cannot connect to the VPN (Other error)</option>
              <option value="no-internet">No internet when connected</option>
              <option value="slow">Slow connection</option>
              <option value="website">Website doesn't work</option>
              <option value="other">Other</option>
            </select>
          </li>
          <li>
            Describe your issue
            <textarea
              style={{ marginTop: '10px' }}
              data-test-id={'contactSupportBody'}
              cols={100}
              rows={10}
              value={formData?.problemBody || ''}
              onChange={onChangeBody}
            />
          </li>
          <li>Please select the information you're comfortable sharing with us</li>
          <li>VPN hostname: {supportData.os_version}</li>
          <li>App version:</li>
          <li>OS version:</li>
          <li>
            The more information you share with us the easier it will be for the support
            staff to help you resolve your issue.

            Support provided with the help of the Guardian team.
          </li>
        </S.List>
        <Button
          level='primary'
          type='accent'
          brand='rewards'
          text='Submit'
          disabled={!isValid}
          onClick={handleSubmit}
        />
      </S.PanelContent>
    </S.Box>
  )
}

export default ContactSupport
