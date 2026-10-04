// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import styles from './search_widget.module.scss'
import SecureLink from '$web-common/SecureLink'

interface SearchResult {
  type: 'search_result' | 'image_result' | 'news_result' | 'video_result'
  title: string
  url: string
  description: string
  thumbnail?: {
    src: string
  }
  age?: string
  meta_url: {
    favicon: string
    path: string
    netloc: string
  }
}

function ChromeImage(props: {
  className?: string
  src?: string
  alt?: string
  style?: React.CSSProperties
  onError?: React.ReactEventHandler<HTMLImageElement>
}) {
  const url =
    props.src && window.location.protocol.startsWith('chrome')
      ? `//image?url=${encodeURIComponent(props.src)}`
      : props.src
  if (!url) return <span className={props.className} />
  return (
    <img
      className={props.className}
      src={url}
      alt={props.alt}
      style={props.style}
      onError={props.onError}
    />
  )
}

function MetaRow(props: { favicon: string; children: React.ReactNode }) {
  return (
    <div className={styles.meta}>
      <ChromeImage src={props.favicon} />
      {props.children}
    </div>
  )
}

function SearchCard(props: { result: SearchResult }) {
  return (
    <SecureLink
      className={styles.searchResult}
      href={props.result.url}
      target='_blank'
    >
      <MetaRow favicon={props.result.meta_url.favicon}>
        <span>
          {props.result.meta_url.netloc} {props.result.meta_url.path}
        </span>
      </MetaRow>
      <div className={styles.content}>
        <span className={styles.title}>{props.result.title}</span>
        <SearchDescription description={props.result.description} />
      </div>
    </SecureLink>
  )
}

const imgHidden: React.CSSProperties = { visibility: 'hidden' }

function DetailCard(props: { result: SearchResult }) {
  const [imgFailed, setImgFailed] = React.useState(false)
  return (
    <SecureLink
      className={styles.detailResult}
      href={props.result.url}
      target='_blank'
      data-type={props.result.type}
    >
      <ChromeImage
        className={styles.thumbnail}
        src={props.result.thumbnail?.src}
        alt={props.result.title}
        style={imgFailed ? imgHidden : undefined}
        onError={() => setImgFailed(true)}
      />
      <div className={styles.content}>
        <MetaRow favicon={props.result.meta_url.favicon}>
          {props.result.meta_url.netloc}
        </MetaRow>
        <span className={styles.title}>{props.result.title}</span>
        {props.result.age && (
          <span className={styles.subtitle}>{props.result.age}</span>
        )}
      </div>
    </SecureLink>
  )
}

function ImageCard(props: { result: SearchResult }) {
  const [failed, setFailed] = React.useState(false)
  if (failed) return null
  return (
    <SecureLink
      className={styles.imageResult}
      href={props.result.url}
      target='_blank'
    >
      <ChromeImage
        className={styles.thumbnail}
        src={props.result.thumbnail?.src}
        alt={props.result.title}
        onError={() => setFailed(true)}
      />
    </SecureLink>
  )
}

function SearchDescription(props: { description: string }) {
  const content = React.useMemo(() => {
    const parts = props.description.split('<strong>')
    return parts.map((p) => {
      const closeTag = '</strong>'
      const index = p.indexOf(closeTag)
      if (index === -1) {
        return p
      }
      return (
        <>
          <span
            key={p.slice(0, index)}
            className={styles.strong}
          >
            {p.slice(0, index)}
          </span>
          {p.slice(index + closeTag.length)}
        </>
      )
    })
  }, [props.description])

  return (
    <div className={styles.description}>
      {content.map((c, i) => (
        <React.Fragment key={i}>{c}</React.Fragment>
      ))}
    </div>
  )
}

function Result(props: { result: SearchResult }) {
  switch (props.result.type) {
    case 'search_result':
      return <SearchCard result={props.result} />
    case 'image_result':
      return <ImageCard result={props.result} />
    case 'news_result':
    case 'video_result':
      return <DetailCard result={props.result} />
    default:
      return null
  }
}

const searchTypes = ['web', 'images', 'videos', 'news'] as const
type SearchTypes = (typeof searchTypes)[number]
const resultTypeToType: Record<SearchResult['type'], SearchTypes> = {
  'search_result': 'web',
  'image_result': 'images',
  'news_result': 'news',
  'video_result': 'videos',
}

const resultTypeIcons: Record<SearchTypes, string> = {
  web: 'search',
  images: 'image',
  videos: 'search-movie',
  news: 'search-news',
}

const getLinkPathForType = (type: SearchTypes) => {
  if (type === 'web') return 'search'
  return type ?? 'search'
}

export default function SearchWidget(props: {
  query: string
  type: SearchTypes
  results: SearchResult[]
}) {
  const [type, setType] = React.useState(() =>
    searchTypes.includes(props.type) ? props.type : 'web',
  )
  const scrollContainerRef = React.useRef<HTMLDivElement>(null)
  const [canScrollLeft, setCanScrollLeft] = React.useState(false)
  const [canScrollRight, setCanScrollRight] = React.useState(false)

  const results = React.useMemo(
    () =>
      props.results.reduce<Record<SearchTypes, SearchResult[]>>(
        (acc, result) => {
          acc[resultTypeToType[result.type] ?? 'web']?.push(result)
          return acc
        },
        {
          web: [],
          images: [],
          news: [],
          videos: [],
        },
      ),
    [props.results],
  )

  // If we have no results for the specified type, pick the first type with results (if any).
  React.useEffect(() => {
    if (!props.results.length) return
    if (!results[type].length) {
      const firstTypeWithResults = searchTypes.find(
        (key) => results[key].length > 0,
      )
      if (firstTypeWithResults) setType(firstTypeWithResults)
    }
  }, [props.results, results, type])

  const resultTypes = React.useMemo(
    () =>
      Object.entries(results)
        .filter(([_, results]) => results.length > 0)
        .map(([type]) => type as SearchTypes),
    [results],
  )

  const checkScrollBounds = React.useCallback(() => {
    const container = scrollContainerRef.current
    if (!container) return

    const { scrollLeft, scrollWidth, clientWidth } = container
    setCanScrollLeft(scrollLeft > 1)
    setCanScrollRight(scrollLeft + clientWidth < scrollWidth - 1)
  }, [])

  React.useEffect(() => {
    const container = scrollContainerRef.current
    if (!container) return

    checkScrollBounds()
    container.addEventListener('scroll', checkScrollBounds)
    return () => container.removeEventListener('scroll', checkScrollBounds)
  }, [checkScrollBounds, props.results])

  const scrollBy = (direction: 'left' | 'right') => {
    if (!scrollContainerRef.current) return
    const scrollAmount =
      scrollContainerRef.current?.querySelector('a')?.getBoundingClientRect()
        .width ?? 0
    scrollContainerRef.current.scrollBy({
      left: direction === 'left' ? -scrollAmount : scrollAmount,
      behavior: 'smooth',
    })
  }

  const searchResults = results[type] ?? []
  const loading = props.results.length === 0

  return (
    <div className={styles.searchWidget}>
      <div
        className={styles.searchResults}
        ref={scrollContainerRef}
      >
        {loading ? (
          <div className={styles.placeholderResult} />
        ) : (
          searchResults.map((result, i) => (
            <Result
              key={i}
              result={result}
            />
          ))
        )}
        <Button
          className={styles.carouselButton}
          size='small'
          kind='outline'
          fab
          onClick={() => scrollBy('left')}
          isDisabled={!canScrollLeft}
        >
          <Icon name='carat-left' />
        </Button>
        <Button
          className={styles.carouselButton}
          size='small'
          kind='outline'
          fab
          onClick={() => scrollBy('right')}
          isDisabled={!canScrollRight}
        >
          <Icon name='carat-right' />
        </Button>
      </div>
      <div className={styles.footer}>
        <SecureLink
          className={styles.query}
          href={`https://search.brave.com/${getLinkPathForType(type)}?q=${encodeURIComponent(props.query)}`}
          target='_blank'
        >
          <Icon
            slot='icon-before'
            name='search'
          />
          <span>{props.query}</span>
        </SecureLink>
        <div className={styles.types}>
          {resultTypes.map((t) => (
            <Button
              key={t}
              size='small'
              kind={t === type ? 'plain' : 'plain-faint'}
              fab
              onClick={() => setType(t)}
            >
              <Icon name={resultTypeIcons[t]} />
            </Button>
          ))}
        </div>
      </div>
    </div>
  )
}
