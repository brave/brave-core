// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { CaratRightIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../../common/locale'
import { Publisher } from '../../../../api/brave_news'
import {
  SettingsRow,
  SettingsText,
  SettingsSectionTitle
} from '../../../../components/default'
import NavigateBack from '../../../../components/default/settings/navigateBack'
import { Props } from './'
import PublisherPrefs from './publisherPrefs'
import * as Styled from './style'

type CategoryListProps = {
  categories: string[]
  setCategory: (category: string) => any
}

function CategoryList (props: CategoryListProps) {
  type ClickFunctions = { [category: string]: () => any}
  const clickFunctions: ClickFunctions = React.useMemo(() => {
    const functions = {}
    for (const category of props.categories) {
      functions[category] = () => props.setCategory(category)
    }
    return functions
  }, [props.categories, props.setCategory])
  return (
    <>
      <SettingsSectionTitle>{getLocale('braveTodaySourcesTitle')}</SettingsSectionTitle>
      {props.categories.map(category => {
        return (
          <SettingsRow key={category} isInteractive={true} onClick={clickFunctions[category]}>
            <SettingsText>{category}</SettingsText>
            <Styled.SourcesCommandIcon>
              <CaratRightIcon />
            </Styled.SourcesCommandIcon>
          </SettingsRow>
        )
      })}
    </>
  )
}

type CategoryProps = {
  category: string
  publishers: Publisher[]
  onBack: () => any
  setPublisherPref: (publisherId: string, enabled: boolean) => any
}

function Category (props: CategoryProps) {
  return (
    <Styled.Section>
      <Styled.StaticPrefs>
        <NavigateBack onBack={props.onBack} />
        <SettingsSectionTitle>{props.category}</SettingsSectionTitle>
      </Styled.StaticPrefs>
      <Styled.PublisherList>
        <PublisherPrefs
          publishers={props.publishers}
          setPublisherPref={props.setPublisherPref}
        />
      </Styled.PublisherList>
    </Styled.Section>
  )
}

const categoryNameAll = getLocale('braveTodayCategoryNameAll')

type SourcesProps = Props & {
  category: string
  setCategory: (category: string) => any
}

export default function Sources (props: SourcesProps) {
  const publishersByCategory = React.useMemo<Map<string, Publisher[]>>(() => {
    const result = new Map<string, Publisher[]>()
    result.set(categoryNameAll, [])
    if (!props.publishers) {
      return result
    }
    for (const publisher of Object.values(props.publishers)) {
      const forAll = result.get(categoryNameAll) || []
      forAll.push(publisher)
      result.set(categoryNameAll, forAll)
      if (publisher.categoryName) {
        const forCategory = result.get(publisher.categoryName) || []
        forCategory.push(publisher)
        result.set(publisher.categoryName, forCategory)
      }
    }
    // Sort all publishers alphabetically
    for (const publishers of result.values()) {
      publishers.sort((a, b) => a.publisherName.toLocaleLowerCase().localeCompare(b.publisherName.toLocaleLowerCase()))
    }
    return result
  }, [props.publishers])
  const onBack = React.useCallback(() => {
    props.setCategory('')
  }, [props.setCategory])
  // No publishers, could be because hasn't opted-in yet
  // TODO(petemill): error state
  if (!props.publishers) {
    return null
  }
  // Category list
  if (!props.category) {
    return (
      <CategoryList
        categories={[...publishersByCategory.keys()]}
        setCategory={props.setCategory}
      />
    )
  }
  const categoryPublishers = publishersByCategory.get(props.category)
  if (!categoryPublishers) {
    return null
  }
  // Category
  return (
    <Category
      category={props.category}
      publishers={categoryPublishers}
      onBack={onBack}
      setPublisherPref={props.setPublisherPref}
    />
  )
}
