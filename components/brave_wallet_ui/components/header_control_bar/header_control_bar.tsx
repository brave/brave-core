// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../common/locale'

// components
import { SearchBar } from '../shared/search-bar'

// styles
import {
  ButtonIcon,
  PortfolioActionButton,
  SearchButtonWrapper
} from '../desktop/views/portfolio/style'
import { Row, Text } from '../shared/style'
import {
  ControlBarWrapper,
  SearchBarWrapper
} from './header_control_bar.styles'

interface Props {
  title: string
  searchValue: string
  onSearchValueChange: (value: string) => void
  actions: Array<{
    buttonIconName: string
    onClick: (() => void) | (() => Promise<void>)
  }>
}

export const HeaderControlBar: React.FC<Props> = ({
  actions,
  onSearchValueChange,
  searchValue,
  title
}) => {
  // state
  const [showSearchBar, setShowSearchBar] = React.useState<boolean>(false)

  // methods
  const handleSearch = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onSearchValueChange(event.target.value)
    },
    [onSearchValueChange]
  )

  const onCloseSearchBar = React.useCallback(() => {
    setShowSearchBar(false)
    onSearchValueChange('')
  }, [onSearchValueChange])

  // render
  return (
    <ControlBarWrapper
      justifyContent='space-between'
      alignItems='center'
      showSearchBar={showSearchBar}
    >
      {!showSearchBar && (
        <Text
          textSize='16px'
          textAlign='left'
          isBold={true}
        >
          {title}
        </Text>
      )}
      <Row width={showSearchBar ? '100%' : 'unset'}>
        <SearchBarWrapper
          margin='0px 12px 0px 0px'
          showSearchBar={showSearchBar}
        >
          <SearchBar
            placeholder={getLocale('braveWalletSearchText')}
            action={handleSearch}
            value={searchValue}
            isV2={true}
          />
        </SearchBarWrapper>
        {showSearchBar && (
          <Row width='unset'>
            <PortfolioActionButton onClick={onCloseSearchBar}>
              <ButtonIcon name='close' />
            </PortfolioActionButton>
          </Row>
        )}
        {!showSearchBar && (
          <Row
            width='unset'
            gap={'12px'}
          >
            <SearchButtonWrapper width='unset'>
              <PortfolioActionButton onClick={() => setShowSearchBar(true)}>
                <ButtonIcon name='search' />
              </PortfolioActionButton>
            </SearchButtonWrapper>

            {actions.map((action) => {
              return (
                <PortfolioActionButton
                  key={action.buttonIconName}
                  onClick={action.onClick}
                >
                  <ButtonIcon name={action.buttonIconName} />
                </PortfolioActionButton>
              )
            })}
          </Row>
        )}
      </Row>
    </ControlBarWrapper>
  )
}
