// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import { Cluster as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import Card from './Card';
import Article from './Article';
import HeroArticle from './Hero';

interface Props {
  info: Info
}

export default function Cluster({ info }: Props) {
  return <>
    <Card>
      Cluster: {info.type} {info.id}
    </Card>
    {info.articles.map(a => a.article
      ? <Article key={a.article.data.url.url} info={a.article} />
      : <HeroArticle key={a.hero!.data.url.url} info={a.hero!} />)}
  </>
}
