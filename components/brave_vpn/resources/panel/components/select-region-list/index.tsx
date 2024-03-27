// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { Radio } from 'brave-ui'
import Button from '$web-components/button'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'

import * as S from './style'
import { Region } from '../../api/panel_browser_api'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'
import { getLocale } from '../../../../../common/locale'
import Flag from '../flag'

function SelectRegion () {
  // TODO(nullhook): Scroll to the selected radio input when this component loads
  // TODO(nullhook): Add search region functionality
  const dispatch = useDispatch()
  const currentRegion = useSelector(state => state.currentRegion)
  const regions = useSelector(state => state.regions)
  const [selectedRegion, setSelectedRegion] = React.useState(currentRegion)
  const ref = React.useRef() as React.MutableRefObject<HTMLButtonElement>

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
    console.error('Selected region is not defined')
  }

  React.useEffect(() => {
    if (ref.current && currentRegion) {
      ref.current.scrollIntoView()
    }
  }, [])

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
            {regions.map((entry: Region, i: number) => (
              <div
                key={i}
                data-value={entry.name}
              >
                <S.RegionLabelButton
                  type="button"
                  aria-describedby="country-name"
                  onClick={handleItemClick.bind(this, entry)}
                >
                  <Flag countryCode={entry.countryIsoCode} />
                  <S.RegionLabel id="country-name">
                    {entry.namePretty}
                  </S.RegionLabel>
                </S.RegionLabelButton>
              </div>
            ))}
          </Radio>
        </S.RegionList>
        <S.ActionArea>
          <Button
            type={'submit'}
            isPrimary
            isCallToAction
            onClick={handleConnect}
          >
            {getLocale('braveVpnConnect')}
          </Button>
        </S.ActionArea>
      </S.PanelContent>
    </S.Box>
  )
}

export default SelectRegion
