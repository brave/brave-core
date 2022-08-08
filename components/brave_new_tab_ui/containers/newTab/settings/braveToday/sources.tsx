// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { CaratRightIcon, PlusIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../../common/locale'
import getBraveNewsController, { Publisher } from '../../../../api/brave_news'
import {
  SettingsRow,
  SettingsText,
  SettingsSectionTitle
} from '../../../../components/default'
import Button, { ButtonIconContainer } from '$web-components/button'
import NavigateBack from '../../../../components/default/settings/navigateBack'
import DirectFeedItemMenu from './directFeedMenu'
import { Props } from './'
import PublisherPrefs from './publisherPrefs'
import * as Styled from './style'
import useManageDirectFeeds, { FeedInputValidity } from './useManageDirectFeeds'

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
const categoryNameDirectFeeds = 'User feeds'

type SourcesProps = Props & {
  category: string
  setCategory: (category: string) => any
}

const useBraveNewsLocale = () => {
  const [locale, setLocale] = React.useState('')
  React.useEffect(() => {
    getBraveNewsController().getLocale().then(({ locale }) => setLocale(locale))
  }, [])
  return locale
}

export default function Sources (props: SourcesProps) {
  const locale = useBraveNewsLocale()

  // Memoisze list of publishers by category
  const publishersByCategory = React.useMemo<Map<string, Publisher[]>>(() => {
    const result = new Map<string, Publisher[]>()
    result.set(categoryNameAll, [])
    if (!props.publishers) {
      return result
    }
    for (const publisher of Object.values(props.publishers)) {
      // If the publisher has a locale (which can only happen in the V2 API) and
      // it doesn't include the current locale, skip over it.
      if (publisher.locales.length !== 0 && !publisher.locales.includes(locale)) {
        continue
      }

      // Do not include user feeds, as they are separated
      if (publisher.categoryName === categoryNameDirectFeeds) {
        continue
      }
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
  }, [props.publishers, locale])
  const {
    userFeeds,
    feedInputIsValid,
    feedInputText,
    onRemoveDirectFeed,
    onChangeFeedInput,
    feedSearchResults,
    onAddSource,
    onSearchForSources
  } = useManageDirectFeeds(props.publishers)
  // Set blank category on navigation back
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
      <>
        <Styled.YourSources>
          <SettingsSectionTitle isLayoutControlled={true}>
            {/* getLocale('braveTodayYourSourcesTitle') */}
            Your Sources
          </SettingsSectionTitle>
          {userFeeds && userFeeds.map(publisher => (
            <SettingsRow key={publisher.publisherId} isInteractive={false} isLayoutControlled={true}>
              <SettingsText title={publisher.feedSource.url}>{publisher.publisherName}</SettingsText>
              <DirectFeedItemMenu key={publisher.publisherId} onRemove={onRemoveDirectFeed.bind(undefined, publisher)} />
            </SettingsRow>
          ))}
          <Styled.AddSourceForm onSubmit={e => { e.preventDefault() }}>
            <Styled.FeedInputLabel>
              Feed URL
              <Styled.FeedInput type={'text'} value={feedInputText} onChange={onChangeFeedInput} />
            </Styled.FeedInputLabel>
            {(feedInputIsValid === FeedInputValidity.NotValid) &&
              <Styled.FeedUrlError>Sorry, we couldn't find a feed at that address.</Styled.FeedUrlError>
            }
            {(feedInputIsValid === FeedInputValidity.IsDuplicate) &&
              <Styled.FeedUrlError>Seems like you already subscribe to that feed.</Styled.FeedUrlError>
            }
            <Styled.YourSourcesAction>
              <Button
                isPrimary
                scale='small'
                type='submit'
                isDisabled={feedInputIsValid !== FeedInputValidity.Valid}
                isLoading={feedInputIsValid === FeedInputValidity.Pending}
                onClick={onSearchForSources}
              >
                Add source
              </Button>
            </Styled.YourSourcesAction>
          </Styled.AddSourceForm>
          {(feedInputIsValid === FeedInputValidity.HasResults) &&
            <Styled.FeedSearchResults>
              Multiple feeds were found:
              <Styled.ResultItems>
              {feedSearchResults.map(result => (
                <Styled.ResultItem key={result.feedUrl.url}>
                  <span title={result.feedUrl.url}>{result.feedTitle}</span>
                  <Button
                    ariaLabel={`Add the feed at ${result.feedUrl.url}`}
                    scale={'tiny'}
                    isDisabled={result.status !== FeedInputValidity.Valid}
                    isLoading={result.status === FeedInputValidity.Pending}
                    onClick={onAddSource.bind(undefined, result.feedUrl.url)}
                  >
                    <ButtonIconContainer>
                      <PlusIcon />
                    </ButtonIconContainer>
                  </Button>
                </Styled.ResultItem>
              ))}
              </Styled.ResultItems>
            </Styled.FeedSearchResults>
          }
        </Styled.YourSources>
        <CategoryList
          categories={[...publishersByCategory.keys()]}
          setCategory={props.setCategory}
        />
      </>
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
