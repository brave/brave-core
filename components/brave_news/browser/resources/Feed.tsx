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

export default function Component({ feed }: Props) {
  const [cardCount, setCardCount] = React.useState(PAGE_SIZE);
  const loadMoreObserver = React.useRef(new IntersectionObserver(entries => {
    // While the feed is loading we get some notifications for fully
    // visible/invisible cards.
    if (!entries.some(i => i.intersectionRatio !== 0 && i.intersectionRatio !== 1)) return

    setCardCount(cardCount => cardCount + PAGE_SIZE);
  }, {
    rootMargin: '0px 0px 900px 0px'
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

      return <div key={getKey(item, index)} ref={index === count - 1 ? setLastCardRef : undefined}>
        {el}
      </div>
    })
  }, [cardCount, feed?.items])

  return <FeedContainer ref={setLastCardRef}>
    {cards}
  </FeedContainer>
}
