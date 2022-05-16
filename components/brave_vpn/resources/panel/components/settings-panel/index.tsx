import * as React from 'react'

import { getLocale } from '../../../../../common/locale'
import * as S from './style'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'
import getPanelBrowserAPI from '../../api/panel_browser_api'

interface Props {
  closeSettingsPanel: React.MouseEventHandler<HTMLButtonElement>
  showContactSupport: React.MouseEventHandler<HTMLAnchorElement>
}

function SettingsPanel (props: Props) {
  const handleClick = (entry: string) => {
    getPanelBrowserAPI().panelHandler.openVpnUI(entry)
  }

  return (
    <S.Box>
      <S.PanelContent>
        <S.PanelHeader>
          <S.BackButton
            type='button'
            onClick={props.closeSettingsPanel}
            aria-label='Close settings'
          >
            <i><CaratStrongLeftIcon /></i>
            <span>{getLocale('braveVpnSettingsPanelHeader')}</span>
          </S.BackButton>
        </S.PanelHeader>
        <S.List>
          <li>
            <a href="#" onClick={handleClick.bind(this, 'manage')}>
              {getLocale('braveVpnManageSubscription')}
            </a>
          </li>
          <li>
            <a href="#" onClick={props.showContactSupport}>
              {getLocale('braveVpnContactSupport')}
            </a>
          </li>
          <li>
            <a href="#" onClick={handleClick.bind(this, 'about')}>
              {getLocale('braveVpnAbout')}
              {' '}
              {getLocale('braveVpn')}
            </a>
          </li>
        </S.List>
      </S.PanelContent>
    </S.Box>
  )
}

export default SettingsPanel
