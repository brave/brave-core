// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { spacing } from "@brave/leo/tokens/css";
import { FeedItemV2, FeedV2, FeedV2Error } from "gen/brave/components/brave_news/common/brave_news.mojom.m";
import * as React from 'react';
import styled from "styled-components";
import Advert from "./feed/Ad";
import Article from "./feed/Article";
import CaughtUp from "./feed/CaughtUp";
import Cluster from "./feed/Cluster";
import Discover from "./feed/Discover";
import HeroArticle from "./feed/Hero";
import LoadingCard from "./feed/LoadingCard";
import NoArticles from "./feed/NoArticles";
import NoFeeds from "./feed/NoFeeds";
import { getHistoryValue, setHistoryState } from "./shared/history";
import NotConnected from "./feed/NotConnected";

// Restoring scroll position is complicated - we have two available strategies:
// 1. Scroll to the same position - as long as the window hasn't been resized,
//    this will bring the user back to exactly where they left.
// 2. If the screen size has changed, scroll the clicked article to the top of
//    the screen.
interface NewsScrollData {
  itemId: string,
  innerWidth: number,
  innerHeight: number,
  scrollPos: number,
}

const CARD_CLASS = 'feed-card'
const FeedContainer = styled.div`
  width: 540px;
  display: flex;
  flex-direction: column;
  gap: ${spacing.xl};

  /* Hide Ad elements, if we weren't able to fill them */
  & .${CARD_CLASS}:empty {
    display: none;
  }
`

interface Props {
  feed: FeedV2 | undefined;
  onViewCountChange?: (newViews: number) => void;
  onSessionStart?: () => void;
}

const getKey = (feedItem: FeedItemV2, index: number): React.Key => {
  if (feedItem.advert) return index
  if (feedItem.article) return feedItem.article.data.url.url
  if (feedItem.hero) return feedItem.hero.data.url.url
  if (feedItem.cluster) return index
  if (feedItem.discover) return index
  throw new Error("Unsupported FeedItem!")
}

export const NEWS_FEED_CLASS = "news-feed"

// The number of cards to load at a time. Making this too high will result in
// jank as all the cards are rendered at once.
const PAGE_SIZE = 25;
const HISTORY_SCROLL_DATA = 'bn-scroll-data'
const HISTORY_CARD_COUNT = 'bn-card-count'

const CARD_COUNT_ATTRIBUTE = 'data-news-card-count'

const HOUR_MS = 60 * 60 * 1000;

const saveScrollPos = (itemId: React.Key) => () => {
  setHistoryState({
    [HISTORY_SCROLL_DATA]: {
      itemId: itemId,
      innerWidth: window.innerWidth,
      innerHeight: window.innerHeight,
      scrollPos: document.scrollingElement?.scrollTop
    }
  })
}

const errors = {
  [FeedV2Error.ConnectionError]: <NotConnected />,
  [FeedV2Error.NoArticles]: <NoArticles />,
  [FeedV2Error.NoFeeds]: <NoFeeds />
}

export default function Component({ feed, onViewCountChange, onSessionStart }: Props) {
  const [cardCount, setCardCount] = React.useState(getHistoryValue(HISTORY_CARD_COUNT, PAGE_SIZE));

  // Store the number of cards we've loaded in history - otherwise when we
  // navigate back we might not be able to scroll down far enough.
  React.useEffect(() => {
    setHistoryState({ [HISTORY_CARD_COUNT]: cardCount })
  }, [cardCount])

  // Track the feed scroll position - if we mount this component somewhere with
  // a scroll data saved in the state, try and restore the scroll position.
  React.useEffect(() => {
    const scrollData = getHistoryValue<NewsScrollData | undefined>(HISTORY_SCROLL_DATA, undefined)
    if (scrollData) {
      // If the viewport size hasn't changed, restore the scroll position.
      // Otherwise, scroll the clicked item into view.
      const scroll = scrollData.innerHeight === window.innerHeight && scrollData.innerWidth === window.innerWidth
        ? () => document.scrollingElement?.scrollTo({ top: scrollData.scrollPos })
        : () => document.querySelector(`[data-id="${scrollData.itemId}"]`)?.scrollIntoView()
      scroll()
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

  // Create intersection observer & relevant state to measure
  // the amount of viewed cards in the session.
  const lastViewedCardCount = React.useRef(0);
  const lastUsageTime = React.useRef<Date | null>(null);
  const viewDepthIntersectionObserver = React.useRef(new IntersectionObserver(entries => {
    const inViewCounts = entries
      .filter(e => e.intersectionRatio === 1)
      .map(e => Number(e.target.getAttribute(CARD_COUNT_ATTRIBUTE)));
    if (!inViewCounts.length) {
      return;
    }
    if (onSessionStart) {
      if (!lastUsageTime.current || (new Date().getTime() - HOUR_MS) > lastUsageTime.current.getTime()) {
        onSessionStart();
      }
    }
    lastUsageTime.current = new Date();
    const largestCardCount = Math.max(...inViewCounts);
    // Ensure we only report increases in scroll depth
    // by comparing to the last scroll card count
    if (lastViewedCardCount.current >= largestCardCount) {
      return;
    }
    const newViews = largestCardCount - lastViewedCardCount.current;
    lastViewedCardCount.current = largestCardCount;
    if (onViewCountChange) {
      onViewCountChange(newViews);
    }
  }, {
    threshold: 1
  }));

  const registerViewDepthObservation = React.useCallback((element: HTMLElement | null) => {
    if (!element) {
      return;
    }
    viewDepthIntersectionObserver.current.observe(element);
  }, []);

  // Only observe the bottom card
  const setLastCardRef = React.useCallback((el: HTMLElement | null) => {
    loadMoreObserver.current.disconnect();
    if (!el) return;

    loadMoreObserver.current.observe(el);
  }, [])

  const cards = React.useMemo(() => {
    const count = Math.min(feed?.items.length ?? 0, cardCount)
    let currentCardCount = 0;
    const setRefAtIndex = (index: number) => (element: HTMLElement | null) => {
      const isLast = index === count - 1
      if (isLast) setLastCardRef(element)
      registerViewDepthObservation(element)
    }

    return feed?.items.slice(0, count).map((item, index) => {
      let el: React.ReactNode

      if (item.advert) {
        el = <Advert info={item.advert} />
      }
      else if (item.article) {
        el = <Article info={item.article} feedDepth={currentCardCount} />
      }
      else if (item.cluster) {
        el = <Cluster info={item.cluster} feedDepth={currentCardCount} />
      }
      else if (item.discover) {
        el = <Discover info={item.discover} />
      }
      else if (item.hero) {
        el = <HeroArticle info={item.hero} feedDepth={currentCardCount} />
      } else {
        throw new Error("Invalid item!" + JSON.stringify(item))
      }

      if (item.cluster) {
        currentCardCount += item.cluster.articles.length;
      } else if (!item.advert) {
        currentCardCount++;
      }

      const key = getKey(item, index)
      return <div className={CARD_CLASS} onClickCapture={saveScrollPos(key)} key={key} data-id={key} {...{ [CARD_COUNT_ATTRIBUTE]: currentCardCount }} ref={setRefAtIndex(index)}>
        {el}
      </div>
    })
  }, [cardCount, feed?.items, registerViewDepthObservation])

  return <FeedContainer className={NEWS_FEED_CLASS}>
    {feed
      ? errors[feed.error!] ?? <>
        {cards}
        <CaughtUp />
      </>
      : <LoadingCard />}
  </FeedContainer>
}
