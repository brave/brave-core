import * as React from 'react'
import * as S from './style'
import StatusIndicator from '../status-indicator'
import Toggle from '../toggle'

function Main () {
  const [isOn, setOn] = React.useState(false)
  const handleClick = () => setOn(state => !state)

  return (
    <S.Box>
      <S.PanelTitle>
        Brave Firewall + VPN
      </S.PanelTitle>
      <S.ToggleBox>
        <Toggle
          isOn={isOn}
          onClick={handleClick}
        />
      </S.ToggleBox>
      <S.StatusIndicatorBox>
        <StatusIndicator
          isConnected={isOn}
        />
      </S.StatusIndicatorBox>
      <S.Button>
        <S.ButtonText>United States</S.ButtonText>
      </S.Button>
    </S.Box>
  )
}

export default Main
