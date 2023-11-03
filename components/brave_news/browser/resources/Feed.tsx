// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { FeedItemV2, FeedV2 } from "gen/brave/components/brave_news/common/brave_news.mojom.m";
import * as React from 'react';
import styled from "styled-components";
import Advert from "./feed/Ad";
import Article from "./feed/Article";
import Cluster from "./feed/Cluster";
import Discover from "./feed/Discover";
import HeroArticle from "./feed/Hero";
import { getHistoryValue, setHistoryState } from "./shared/history";

const FeedContainer = styled.div`
  max-width: 540px;
  display: flex;
  flex-direction: column;
  gap: 12px;
`

interface Props {
  feed: FeedV2 | undefined;
}

const getKey = (feedItem: FeedItemV2, index: number): React.Key => {
  if (feedItem.advert) return index
  if (feedItem.article) return feedItem.article.data.url.url
  if (feedItem.hero) return feedItem.hero.data.url.url
  if (feedItem.cluster) return index
  if (feedItem.discover) return index
  throw new Error("Unsupported FeedItem!")
}

// The number of cards to load at a time. Making this too high will result in
// jank as all the cards are rendered at once.
const PAGE_SIZE = 25;
const CARD_CLASS = 'feed-card'

export default function Component({ feed }: Props) {
  const [cardCount, setCardCount] = React.useState(getHistoryValue('bn-card-count', PAGE_SIZE));

  // Store the number of cards we've loaded in history - otherwise when we
  // navigate back we might not be able to scroll down far enough.
  React.useEffect(() => {
    setHistoryState({ 'bn-card-count': cardCount })
  }, [cardCount])


  // Track the feed scroll position - if we mount this component somewhere with
  // a bn-scroll-pos saved in the state, try and restore the scroll position.
  React.useEffect(() => {
    const scrollPos = getHistoryValue('bn-scroll-pos', 0)
    if (scrollPos) {
      // TODO: Once we work out what else is scrolling us, this should be okay to remove.
      setTimeout(() => {
        document.scrollingElement!.scrollTo({
          behavior: 'smooth',
          top: scrollPos
        })
      })
    }

    const handler = () => {
      setHistoryState({ 'bn-scroll-pos': document.scrollingElement!.scrollTop })
    }
    document.addEventListener('scroll', handler, { passive: true })
    return () => {
      document.removeEventListener('scroll', handler)
    }
  }, [])

  const loadMoreObserver = React.useRef(new IntersectionObserver(entries => {
    // While the feed is loading we get some notifications for fully
    // invisible cards.
    if (!entries.some(i => i.target?.classList.contains(CARD_CLASS) && i.intersectionRatio !== 0)) return
    setCardCount(cardCount => cardCount + PAGE_SIZE);
  }, {
    rootMargin: '0px 0px 1000px 0px',
  }))

  // Only observe the bottom card
  const setLastCardRef = React.useCallback((el: HTMLElement | null) => {
    loadMoreObserver.current.disconnect();
    if (!el) return;

    loadMoreObserver.current.observe(el);
  }, [])
  const cards = React.useMemo(() => {
    const count = Math.min(feed?.items.length ?? 0, cardCount)
    return feed?.items.slice(0, count).map((item, index) => {
      let el: React.ReactNode

      if (item.advert) {
        el = <Advert info={item.advert} />
      }
      else if (item.article) {
        el = <Article info={item.article} />
      }
      else if (item.cluster) {
        el = <Cluster info={item.cluster} />
      }
      else if (item.discover) {
        el = <Discover info={item.discover} />
      }
      else if (item.hero) {
        el = <HeroArticle info={item.hero} />
      } else {
        throw new Error("Invalid item!" + JSON.stringify(item))
      }

      return <div className={CARD_CLASS} key={getKey(item, index)} ref={index === count - 1 ? setLastCardRef : undefined}>
        {el}
      </div>
    })
  }, [cardCount, feed?.items])

  return <FeedContainer ref={setLastCardRef}>
    {cards}
  </FeedContainer>
}
