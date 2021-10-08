import * as React from 'react'
import * as S from './style'
import StatusIndicator from '../../components/status-indicator'
import PanelBox from '../../components/panel-box'
import Toggle from '../../components/toggle'
import { SettingsAdvancedIcon, CaratStrongRightIcon } from 'brave-ui/components/icons'
import { ConnectionState } from '../../api/panel_browser_api'
import { Region } from '../../api/region_interface'
interface Props {
  isOn: boolean
  status: ConnectionState
  region: Region
  onToggleClick: () => void
  onSelectRegionButtonClick: () => void
}

function MainPanel (props: Props) {
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
        <S.ToggleBox>
          <Toggle
            isOn={props.isOn}
            onClick={props.onToggleClick}
          />
        </S.ToggleBox>
        <S.StatusIndicatorBox>
          <StatusIndicator
            status={props.status}
          />
        </S.StatusIndicatorBox>
        <S.RegionSelectorButton
          type='button'
          onClick={props.onSelectRegionButtonClick}
        >
          <S.RegionLabel>{props.region.namePretty}</S.RegionLabel>
          <CaratStrongRightIcon />
        </S.RegionSelectorButton>
      </S.PanelContent>
    </PanelBox>
  )
}

export default MainPanel
