// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import Card from './Card';
import { HeroArticle as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';

interface Props {
  info: Info
}

export default function HeroArticle({ info }: Props) {
  return <Card onClick={() => window.open(info.data.url.url, '_blank', 'noopener noreferrer')}>
    <h2>Hero: {info.data.title}</h2>
    <div>{info.data.description}</div>
  </Card>
}
