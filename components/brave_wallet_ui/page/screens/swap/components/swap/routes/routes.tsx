// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'

// Types
import { QuoteOption, RouteTagsType } from '../../../constants/types'

// Selectors
import {
  useSafeUISelector //
} from '../../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../../common/selectors'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Components
import { RouteOption } from './route_option'

// Styles
import { CaratDownIcon, StyledWrapper } from './routes.style'
import {
  LeoSquaredButton,
  Row,
  ScrollableColumn,
  Text
} from '../../../../../../components/shared/style'

interface Props {
  quoteOptions: QuoteOption[]
  selectedQuoteOptionId: string
  onSelectQuoteOption: (id: string) => void
}

export const Routes = (props: Props) => {
  const { quoteOptions, selectedQuoteOptionId, onSelectQuoteOption } = props

  // State
  const [userSelectedQuoteOptionId, setUserSelectedQuoteOptionId] =
    React.useState(selectedQuoteOptionId)
  const [showSortByMenu, setShowSortByMenu] = React.useState(false)
  const [sortBy, setSortBy] = React.useState<RouteTagsType>('CHEAPEST')

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // Memos
  const sortedQuoteOptions = React.useMemo(() => {
    let quoteList = Array.from(quoteOptions)
    const indexOfSortBy = quoteList.findIndex((option) =>
      option.tags.includes(sortBy)
    )
    quoteList.push(...quoteList.splice(0, indexOfSortBy))
    return quoteList
  }, [sortBy, quoteOptions])

  return (
    <StyledWrapper
      fullWidth={true}
      fullHeight={true}
      alignItems='flex-start'
      justifyContent='flex-start'
    >
      <Row
        marginBottom={isPanel ? '14px' : '24px'}
        justifyContent={isPanel ? 'center' : 'flex-start'}
        padding='0px 24px'
      >
        <Text
          textSize={isPanel ? '16px' : '22px'}
          isBold={true}
          textColor='primary'
        >
          {getLocale('braveWalletRoutes')}
        </Text>
      </Row>
      <Row
        marginBottom='14px'
        justifyContent='space-between'
        padding='0px 16px'
      >
        <Text
          textSize='12px'
          isBold={true}
          textColor='primary'
        >
          {getLocale('braveWalletSortBy')}
        </Text>
        <ButtonMenu
          isOpen={showSortByMenu}
          onChange={({ isOpen }) => setShowSortByMenu(isOpen)}
          onClose={() => setShowSortByMenu(false)}
        >
          <Row
            slot='anchor-content'
            width='unset'
            gap='8px'
          >
            <Text
              textSize='12px'
              isBold={true}
              textColor='secondary'
            >
              {getLocale(
                sortBy === 'CHEAPEST'
                  ? 'braveWalletCheapest'
                  : 'braveWalletFastest'
              )}
            </Text>
            <CaratDownIcon isExpanded={showSortByMenu} />
          </Row>
          <leo-menu-item onClick={() => setSortBy('FASTEST')}>
            {getLocale('braveWalletFastest')}
          </leo-menu-item>
          <leo-menu-item onClick={() => setSortBy('CHEAPEST')}>
            {getLocale('braveWalletCheapest')}
          </leo-menu-item>
        </ButtonMenu>
      </Row>
      <ScrollableColumn
        gap='8px'
        padding='2px 16px'
      >
        {sortedQuoteOptions.map((option) => (
          <RouteOption
            isSelected={option.id === userSelectedQuoteOptionId}
            onClickOption={() => setUserSelectedQuoteOptionId(option.id)}
            option={option}
            key={option.id}
          />
        ))}
      </ScrollableColumn>
      <Row padding='16px'>
        <LeoSquaredButton
          onClick={() => onSelectQuoteOption(userSelectedQuoteOptionId)}
          size='large'
        >
          {getLocale('braveWalletUpdate')}
        </LeoSquaredButton>
      </Row>
    </StyledWrapper>
  )
}
