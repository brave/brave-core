// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

import ProgressRing from '@brave/leo/react/progressRing'

// Constants & Options
import { ExploreNavOptions } from '../../../../options/nav-options'

// Utils
import { getLocale } from '../../../../../common/locale'
import { makeDappDetailsRoute } from '../../../../utils/routes-utils'

// Hooks
import { useGetTopDappsQuery } from '../../../../common/slices/api.slice'

// Components
import {
  SegmentedControl //
} from '../../../shared/segmented_control/segmented_control'
import {
  HeaderControlBar //
} from '../../../header_control_bar/header_control_bar'
import { DividerLine } from '../../../extension/divider'

// Styles
import { Column, Row } from '../../../shared/style'
import { ControlsRow } from '../portfolio/style'
import { VirtualizedDappsList } from './virtualized_dapps_list'

export const ExploreWeb3View = () => {
  // routing
  const history = useHistory()

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')

  // queries
  const { isLoading, data: topDapps } = useGetTopDappsQuery(undefined)

  // memos
  const controls = React.useMemo(() => {
    return [
      {
        buttonIconName: 'funnel',
        onClick: () => {
          // TODO
        }
      }
    ]
  }, [])

  const searchedDapps = React.useMemo(() => {
    if (!topDapps) {
      return []
    }

    const searchValueLower = searchValue.toLowerCase().trim()

    if (!searchValueLower) {
      return topDapps
    }

    return topDapps.filter((dapp) => {
      return (
        dapp.name.toLowerCase().includes(searchValueLower) ||
        dapp.description.toLowerCase().includes(searchValueLower)
      )
    })
  }, [topDapps, searchValue])

  // render
  if (isLoading || !topDapps) {
    return (
      <Column
        fullHeight
        fullWidth
      >
        <ProgressRing />
      </Column>
    )
  }

  return (
    <Column
      fullHeight
      fullWidth
      justifyContent='flex-start'
    >
      <ControlsRow>
        <SegmentedControl
          navOptions={ExploreNavOptions}
          width={384}
        />
      </ControlsRow>
      <HeaderControlBar
        actions={controls}
        onSearchValueChange={setSearchValue}
        searchValue={searchValue}
        title={getLocale('braveWalletTopNavMarket')}
      />

      <DividerLine />

      {searchedDapps.length ? (
        <VirtualizedDappsList
          dappsList={searchedDapps}
          onClickDapp={(dappId) => {
            history.push(makeDappDetailsRoute(dappId.toString()))
          }}
        />
      ) : (
        <Row>
          <h2>
            {/* TODO */}
            No dapps found
          </h2>
        </Row>
      )}
    </Column>
  )
}
