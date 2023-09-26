// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
// import { FixedSizeList as List } from 'react-window'

// types
import { BraveWallet } from '../../../../../../constants/types'

// components
import { Dapp } from '../dapp/dapp'

// styles
import {
  CategoryTitle,
  CategoryWrapper,
  DappGrid,
  ListToggleButton,
  LoadingRing,
  StyledWrapper
} from './dapp-category-list.styles'
import { Row } from '../../../../../shared/style'

interface CategoryList {
  [category: string]: BraveWallet.Dapp[]
}

interface CategoryListProps {
  isLoading: boolean
  data: CategoryList
}

const Category = ({
  category,
  dapps,
  expanded,
  onShowMore
}: {
  category: string
  dapps: BraveWallet.Dapp[]
  expanded: boolean
  onShowMore: () => void
}) => {
  return (
    <CategoryWrapper>
      <CategoryTitle>{category}</CategoryTitle>
      <DappGrid>
        {dapps.map((dapp) => (
          <Dapp key={`${category}-${dapp.id}`} dapp={dapp} />
        ))}
      </DappGrid>
      <ListToggleButton onClick={onShowMore}>
        {expanded ? 'Show less' : 'Show more'}
      </ListToggleButton>
    </CategoryWrapper>
  )
}

const DappCategoryList = ({ isLoading, data }: CategoryListProps) => {
  const [expandedItem, setExpandedItem] = React.useState<number>()
  const categories = Object.keys(data)

  const toggleItem = (index: number) => {
    if (expandedItem === index) {
      // Clicking on an already expanded item will collapse it
      setExpandedItem(undefined)
    } else {
      setExpandedItem(index)
    }
  }

  return (
    <StyledWrapper>
      {!isLoading ? (
        <>
          {categories.map((category, index) => (
            <Category
              key={category}
              category={category}
              dapps={
                expandedItem === index
                  ? data[category]
                  : data[category].slice(0, 8)
              }
              expanded={expandedItem === index}
              onShowMore={() => toggleItem(index)}
            />
          ))}
        </>
      ) : (
        <Row alignItems='center' justifyContent='center' width='100%' margin='48px 0 0 0'>
          <LoadingRing />
        </Row>
      )}
    </StyledWrapper>
  )
}

export default DappCategoryList
