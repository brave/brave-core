import * as React from 'react'
import { SettingsAdvancedIcon, CaratStrongRightIcon } from 'brave-ui/components/icons'

import * as S from './style'
import SelectRegionList from '../select-region-list'
import PanelBox from '../panel-box'
import Toggle from '../toggle'
import ErrorPanel from '../error-panel'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'

function MainPanel () {
  const dispatch = useDispatch()
  const currentRegion = useSelector(state => state.currentRegion)
  const hasError = useSelector(state => state.hasError)
  const isSelectingRegion = useSelector(state => state.isSelectingRegion)

  const onSelectRegionButtonClick = () => {
    dispatch(Actions.toggleRegionSelector(true))
  }

  if (isSelectingRegion) {
    return(<SelectRegionList />)
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
          >
            <SettingsAdvancedIcon />
          </S.SettingsButton>
        </S.PanelHeader>
        <S.PanelTitle>Brave Firewall + VPN</S.PanelTitle>
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
