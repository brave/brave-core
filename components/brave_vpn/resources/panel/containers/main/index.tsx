import * as React from 'react'
import * as S from './style'
import StatusIndicator from '../../components/status-indicator'
import PanelBox from '../../components/panel-box'
import Toggle from '../../components/toggle'
import RegionSelectorButton from '../../components/region-selector-button'
import SelectRegion from '../../components/select-region'
import { SettingsAdvancedIcon } from 'brave-ui/components/icons'

function Main () {
  const [isOn, setOn] = React.useState(false)
  const [isSelectingRegion, setSelectingRegion] = React.useState(false)

  const handleToggleClick = () => setOn(state => !state)
  const handleSelectRegionButtonClick = () => setSelectingRegion(true)
  const handleOnDone = () => setSelectingRegion(false)

  if (isSelectingRegion) {
    return <SelectRegion onDone={handleOnDone} />
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
        <S.ToggleBox>
          <Toggle
            isOn={isOn}
            onClick={handleToggleClick}
          />
        </S.ToggleBox>
        <S.StatusIndicatorBox>
          <StatusIndicator
            isConnected={isOn}
          />
        </S.StatusIndicatorBox>
        <RegionSelectorButton
          region='Tokyo'
          onClick={handleSelectRegionButtonClick}
        />
      </S.PanelContent>
    </PanelBox>
  )
}

export default Main
