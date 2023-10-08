// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react';
import { useInspectContext } from './context';
import Advert from './feed/Ad';
import Article from './feed/Article';
import Cluster from './feed/Cluster';
import Discover from './feed/Discover'
import HeroArticle from './feed/Hero';
import styled from 'styled-components';
import { spacing } from '@brave/leo/tokens/css';

interface Props {
}

const FeedContainer = styled.div`
  display: flex;
  flex-direction: column;
  gap: 12px;
`

const Container = styled.div`
  display: flex;
  flex-direction: column;
  gap: ${spacing.m};
  max-width: 560px;
  margin: 0 auto;
`

export default function FeedPage(props: Props) {
  const { feed, truncate } = useInspectContext();
  return <Container>
    <h2>The Feed ({feed?.items.length} items. Truncated at {truncate})</h2>
    <FeedContainer>
      {feed?.items.slice(0, truncate).map((item, index) => {
        if (item.advert) {
          return <Advert info={item.advert} key={index} />
        }
        if (item.article) {
          return <Article info={item.article} key={item.article.data.url.url} />
        }
        if (item.cluster) {
          return <Cluster info={item.cluster} key={index} />
        }
        if (item.discover) {
          return <Discover info={item.discover} key={index} />
        }
        if (item.hero) {
          return <HeroArticle info={item.hero} key={item.hero.data.url.url} />
        }

        throw new Error("Invalid item!" + JSON.stringify(item))
      })}
    </FeedContainer>
  </Container>
}
