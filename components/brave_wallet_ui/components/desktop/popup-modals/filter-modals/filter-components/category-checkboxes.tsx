// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Checkbox from '@brave/leo/react/checkbox'

// Utils
import { getLocale } from '../../../../../../common/locale'
import { capitalizeFirstLetter } from '../../../../../utils/string-utils'

// Styled Components
import {
  CheckboxText,
  CheckboxRow,
  CheckboxWrapper,
  SelectAllButton,
  Title
} from './filter-components.style'
import { Row } from '../../../../shared/style'

interface Props {
  categories: string[]
  title: string
  marginBottom: number
  isSelectAll: boolean
  onSelectOrDeselectAllCategories: () => void
  isCategoryFilteredOut: (category: string) => boolean
  onCheckCategory: (category: string) => void
}

export const CategoryCheckboxes = (props: Props) => {
  const {
    title,
    categories,
    isSelectAll,
    marginBottom,
    onCheckCategory,
    isCategoryFilteredOut,
    onSelectOrDeselectAllCategories
  } = props

  return (
    <>
      <Row
        marginBottom={8}
        justifyContent='space-between'
      >
        <Title
          textSize='16px'
          isBold={true}
        >
          {title}
        </Title>
        <SelectAllButton onClick={onSelectOrDeselectAllCategories}>
          {isSelectAll
            ? getLocale('braveWalletSelectAll')
            : getLocale('braveWalletDeselectAll')}
        </SelectAllButton>
      </Row>{' '}
      <CheckboxRow
        justifyContent='space-between'
        marginBottom={marginBottom}
      >
        {categories.map((category) => (
          <CheckboxWrapper
            width='unset'
            justifyContent='flex-start'
            marginBottom={16}
            key={category}
          >
            <Checkbox
              checked={!isCategoryFilteredOut(category)}
              onChange={() => onCheckCategory(category)}
            >
              <CheckboxText
                textSize='14px'
                isBold={false}
              >
                {capitalizeFirstLetter(category)}
              </CheckboxText>
            </Checkbox>
          </CheckboxWrapper>
        ))}
      </CheckboxRow>
    </>
  )
}
