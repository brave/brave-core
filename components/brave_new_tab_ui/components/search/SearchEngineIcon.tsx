// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from "react";
import { icon } from "@brave/leo/tokens/css/variables";
import styled from "styled-components";
import { SearchEngineInfo } from "../../api/background";
import Icon from "@brave/leo/react/icon";
import { loadTimeData } from '$web-common/loadTimeData'

const isBraveOriginBranded = loadTimeData.getBoolean('isBraveOriginBranded')

// Mapping of search engine host to Nala icon for the icons we bundle in the
// browser. For Brave Origin, use the outline icon instead of the full color
// Brave logo.
const braveSearchIcon = isBraveOriginBranded
  ? 'brave-icon-only-face'
  : 'social-brave-release-favicon-fullheight-color'

const icons = {
  // The Google search provider has the empty origin :O
  '': 'google-color',
  'duckduckgo.com': 'duckduckgo-color',
  'search.brave.com': braveSearchIcon,
  'www.bing.com': 'bing-color',
  'www.qwant.com': 'qwant-color',
  'www.startpage.com': 'startpage-color',
  'search.yahoo.com': 'yahoo-color',
  'yandex.com': 'yandex-color',
  'ecosia.org': 'ecosia-color',
}

const hide = { opacity: 0 }
// Component which hides the image until it successfully loads (if ever).
export function MaybeImage(props: React.DetailedHTMLProps<React.ImgHTMLAttributes<HTMLImageElement>, HTMLImageElement>) {
  const { src, ...rest } = props
  const [loaded, setLoaded] = React.useState(false)
  const onLoad = React.useCallback(() => setLoaded(true), []);

  React.useEffect(() => {
    setLoaded(false)
  }, [src])
  return <img {...rest} style={loaded ? undefined : hide} onLoad={onLoad} src={`chrome://image?url=${encodeURIComponent(src ?? '')}`} />
}

export function SearchEngineIcon(props: { engine?: SearchEngineInfo, className?: string }) {
  if (!props.engine) return null

  const nalaIcon = icons[props.engine.host as keyof typeof icons]

  return nalaIcon
    ? <Icon name={nalaIcon} className={props.className} />
    : <MaybeImage src={props.engine.faviconUrl.url} className={props.className} />
}

export const MediumSearchEngineIcon = styled(SearchEngineIcon)`
  --leo-icon-size: ${icon.m};

  width: ${icon.m};
  height: ${icon.m};
`
