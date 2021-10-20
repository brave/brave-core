import * as React from 'react'
import { Button, Radio } from 'brave-ui'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'

import * as S from './style'
import { Region } from '../../api/panel_browser_api'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'
import locale from '../../constants/locale'

function SelectRegion () {
  // TODO(nullhook): Scroll to the selected radio input when this component loads
  // TODO(nullhook): Add search region functionality
  const dispatch = useDispatch()
  const currentRegion = useSelector(state => state.currentRegion)
  const regions = useSelector(state => state.regions)
  const [selectedRegion, setSelectedRegion] = React.useState(currentRegion)

  const handleGoBackClick = () => {
    dispatch(Actions.toggleRegionSelector(false))
  }

  const handleItemClick = (currentRegion: Region) => {
    // Using a local state will help to keep the selection
    // consistent, if the user exits without pressing connect
    setSelectedRegion(currentRegion)
  }

  const handleConnect = () => {
    dispatch(Actions.connectToNewRegion(selectedRegion))
  }

  if (!selectedRegion) {
    console.error(`Selected region is not defined`)
  }

  return (
    <S.Box>
      <S.PanelContent>
        <S.PanelHeader>
          <S.BackButton
            type='button'
            onClick={handleGoBackClick}
          >
            <CaratStrongLeftIcon />
          </S.BackButton>
        </S.PanelHeader>
        <S.RegionList>
          <Radio
            value={selectedRegion ? { [selectedRegion?.name]: true } : {}}
            size={'small'}
            disabled={false}
          >
            {regions?.map((entry: Region, i: number) => (
              <div
                key={i}
                data-value={entry.name}
              >
                <S.RegionLabel
                  onClick={handleItemClick.bind(this, entry)}
                >
                  {entry.namePretty}
                </S.RegionLabel>
              </div>
            ))}
          </Radio>
        </S.RegionList>
        <S.ActionArea>
          <Button
            onClick={handleConnect}
            level='primary'
            type='accent'
            brand='rewards'
            text={locale.connectLabel}
          />
        </S.ActionArea>
      </S.PanelContent>
    </S.Box>
  )
}

export default SelectRegion
