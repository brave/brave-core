import * as React from 'react'
import styles from './search_widget.module.scss'
import usePromise from '$web-common/usePromise'
import { loadTimeData } from '$web-common/loadTimeData'
import ProgressRing from '@brave/leo/react/progressRing'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

interface Thumbnail {
  src: string,
  original?: string,
  logo?: boolean
  width?: number,
  height?: number,
}

interface Query {
  original: string
}

interface SearchResult<T extends string> {
  type: T,
  title: string
  url: string
  description: string
  thumbnail: Thumbnail
  age?: string
  meta_url: {
    favicon: string,
    path: string,
    scheme: string,
    hostname: string,
    net_loc: string
  }
}

interface SearchResponse {
  type: "search",
  query: Query,
  web: {
    results: SearchResult<"search_result">[]
  }
}

interface NewsResponse {
  type: "news",
  query: Query,
  results: SearchResult<"news_result">[]
}

interface ImagesResponse {
  type: "images",
  query: Query,
  results: SearchResult<"image_result">[]
}

interface VideosResponse {
  type: "videos",
  query: Query,
  results: SearchResult<"video_result">[]
}

type Result = {
  web: SearchResponse,
  news: NewsResponse,
  images: ImagesResponse,
  videos: VideosResponse,
}

function fetchResults<T extends keyof Result>(type: T, query: string): Promise<Result[T]> {
  return fetch(`${loadTimeData.getString('apiHost')}/search-api/${type}/search?q=${encodeURIComponent(query)}`)
    .then(r => r.json())
}

function ChromeImage(props: { className?: string, src: string, alt?: string }) {
  const url = window.location.protocol.startsWith('chrome')
    ? `//image?url=${encodeURIComponent(props.src)}`
    : props.src
  return <img className={props.className} src={url} alt={props.alt} />
}

function MetaRow(props: { favicon: string, children: React.ReactNode }) {
  return <div className={styles.meta}>
    <ChromeImage src={props.favicon} />
    {props.children}
  </div>
}

function SearchCard(props: { result: SearchResult<"search_result"> }) {
  return <a className={styles.searchResult} href={props.result.url} target='_blank'>
    <MetaRow favicon={props.result.meta_url.favicon}>
      <span>{props.result.meta_url.hostname} {props.result.meta_url.path}</span>
    </MetaRow>
    <div className={styles.content}>
      <span className={styles.title}>{props.result.title}</span>
      <SearchDescription description={props.result.description} />
    </div>
  </a>
}

function DetailCard(props: { result: SearchResult<"video_result" | "news_result"> }) {
  return <a className={styles.detailResult} href={props.result.url} target='_blank' data-type={props.result.type}>
    <ChromeImage className={styles.thumbnail} src={props.result.thumbnail.src} alt={props.result.title} />
    <div className={styles.content}>
      <MetaRow favicon={props.result.meta_url.favicon}>
        {props.result.meta_url.hostname}
      </MetaRow>
      <span className={styles.title}>{props.result.title}</span>
      {props.result.age && <span className={styles.subtitle}>{props.result.age}</span>}
    </div>
  </a>
}

function ImageCard(props: { result: SearchResult<"image_result"> }) {
  return <a className={styles.imageResult} href={props.result.url} target='_blank'>
    <ChromeImage className={styles.thumbnail} src={props.result.thumbnail.original ?? props.result.thumbnail.src} alt={props.result.title} />
  </a>
}

function SearchDescription(props: { description: string }) {
  const content = React.useMemo(() => {
    const parts = props.description.split('<strong>')
    return parts.map(p => {
      const index = p.indexOf('</strong>')
      if (index === -1) {
        return p
      }
      return <span key={p.slice(0, index)} className={styles.strong}>{p.slice(0, index)}</span>
    })
  }, [props.description])

  return <div className={styles.description}>
    {content.map((c, i) => <React.Fragment key={i}>{c}</React.Fragment>)}
  </div>
}

function Result(props: { result: SearchResult<string> }) {
  switch (props.result.type) {
    case "search_result":
      return <SearchCard result={props.result as SearchResult<"search_result">} />
    case "image_result":
      return <ImageCard result={props.result as SearchResult<"image_result">} />
    case "news_result":
    case "video_result":
      return <DetailCard result={props.result as SearchResult<"video_result" | "news_result">} />
    default:
      return null
  }
}

export default function SearchWidget(props: { query: string, type: 'web' | 'images' | 'videos' | 'news' }) {
  const [type, setType] = React.useState(props.type)
  const { result, loading } = usePromise(() => fetchResults(type, props.query),
    [props.query, type])
  const scrollContainerRef = React.useRef<HTMLDivElement>(null)
  const [canScrollLeft, setCanScrollLeft] = React.useState(false)
  const [canScrollRight, setCanScrollRight] = React.useState(false)

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
  }, [checkScrollBounds, result])

  const scrollBy = (direction: 'left' | 'right') => {
    if (!scrollContainerRef.current) return
    const scrollAmount = scrollContainerRef.current?.querySelector('a')?.getBoundingClientRect().width ?? 0
    scrollContainerRef.current.scrollBy({
      left: direction === 'left' ? -scrollAmount : scrollAmount,
      behavior: 'smooth'
    })
  }

  const results = result
    ? 'results' in result
      ? result.results
      : result.web.results
    : [];
  return (<div className={styles.searchWidget}>
    <div className={styles.searchResults} ref={scrollContainerRef}>
      {results && !loading
        ? results.map((result, i) => <Result key={i} result={result} />)
        : <ProgressRing />}
      <Button className={styles.carouselButton} size='small' kind='outline' fab onClick={() => scrollBy('left')} isDisabled={!canScrollLeft}>
        <Icon name='carat-left' />
      </Button>
      <Button className={styles.carouselButton} size='small' kind='outline' fab onClick={() => scrollBy('right')} isDisabled={!canScrollRight}>
        <Icon name='carat-right' />
      </Button>
    </div>
    <div className={styles.footer}>
      <a className={styles.query} href={`https://search.brave.com/search?q=${encodeURIComponent(props.query)}`} target='_blank'>
        <Icon slot='icon-before' name='search' />
        <span>{props.query}</span>
      </a>
      <div className={styles.types}>
        <Button size='small' kind={type === 'web' ? 'plain' : 'plain-faint'} fab onClick={() => setType('web')}>
          <Icon name='search' />
        </Button>
        <Button size='small' kind={type === 'images' ? 'plain' : 'plain-faint'} fab onClick={() => setType('images')}>
          <Icon name='image' />
        </Button>
        <Button size='small' kind={type === 'videos' ? 'plain' : 'plain-faint'} fab onClick={() => setType('videos')}>
          <Icon name='search-movie' />
        </Button>
        <Button size='small' kind={type === 'news' ? 'plain' : 'plain-faint'} fab onClick={() => setType('news')}>
          <Icon name='search-news' />
        </Button>
      </div>
    </div>
  </div >)
}
