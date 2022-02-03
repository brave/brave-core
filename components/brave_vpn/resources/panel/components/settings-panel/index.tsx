import * as React from 'react'

import { getLocale } from '../../../../../common/locale'
import { useSelector } from '../../state/hooks'
import * as S from './style'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'

interface Props {
  closeSettingsPanel: React.MouseEventHandler<HTMLButtonElement>
}

function SettingsPanel (props: Props) {
  const productUrls = useSelector(state => state.productUrls)

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
            <span>{getLocale('braveVpnSettings')}</span>
          </S.BackButton>
        </S.PanelHeader>
        <S.List>
          <li>
            <a href={productUrls?.manage} target='_blank'>
              {getLocale('braveVpnManageSubscription')}
            </a>
          </li>
          <li>
            <a href={productUrls?.feedback} target='_blank'>
              {getLocale('braveVpnContactSupport')}
            </a>
          </li>
          <li>
            <a href={productUrls?.about} target='_blank'>
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
