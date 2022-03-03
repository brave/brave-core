import * as React from 'react'
import { SettingsAdvancedIcon, CaratStrongRightIcon } from 'brave-ui/components/icons'

import * as S from './style'
import { getLocale } from '../../../../../common/locale'
import SelectRegionList from '../select-region-list'
import PanelBox from '../panel-box'
import Toggle from '../toggle'
import ErrorPanel from '../error-panel'
import SettingsPanel from '../settings-panel'
import ContactSupport from '../contact-support'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'

function MainPanel () {
  const dispatch = useDispatch()
  const [isSettingsPanelVisible, setSettingsPanelVisible] = React.useState(false)
  const [isContactSupportVisible, setContactSupportVisible] = React.useState(false)
  const currentRegion = useSelector(state => state.currentRegion)
  const hasError = useSelector(state => state.hasError)
  const isSelectingRegion = useSelector(state => state.isSelectingRegion)

  const onSelectRegionButtonClick = () => {
    dispatch(Actions.toggleRegionSelector(true))
  }

  const handleSettingsButtonClick = () => setSettingsPanelVisible(true)
  const closeSettingsPanel = () => setSettingsPanelVisible(false)

  const showContactSupport = () => setContactSupportVisible(true)
  const closeContactSupport = () => setContactSupportVisible(false)

  if (isContactSupportVisible) {
    return (<ContactSupport
      onCloseContactSupport={closeContactSupport}
    />)
  }

  if (isSettingsPanelVisible) {
    return (<SettingsPanel
      closeSettingsPanel={closeSettingsPanel}
      showContactSupport={showContactSupport}
    />)
  }

  if (isSelectingRegion) {
    return (<SelectRegionList />)
  }

  if (hasError) {
    return (<ErrorPanel />)
  }

  return (
    <PanelBox>
      <S.PanelContent>
        <S.PanelHeader>
          <S.SettingsButton
            type='button'
            onClick={handleSettingsButtonClick}
          >
            <SettingsAdvancedIcon />
          </S.SettingsButton>
        </S.PanelHeader>
        <S.PanelTitle>{getLocale('braveVpn')}</S.PanelTitle>
        <Toggle />
        <S.RegionSelectorButton
          type='button'
          onClick={onSelectRegionButtonClick}
        >
          <S.RegionLabel>{currentRegion?.namePretty}</S.RegionLabel>
          <CaratStrongRightIcon />
        </S.RegionSelectorButton>
      </S.PanelContent>
    </PanelBox>
  )
}

export default MainPanel
