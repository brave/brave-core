// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from "react";
import { useUnpaddedImageUrl } from "../../../brave_news/browser/resources/shared/useUnpaddedImageUrl";
import { icon } from "@brave/leo/tokens/css/variables";
import styled from "styled-components";

const hide = { opacity: 0 }
function SearchEngineIcon(props: React.DetailedHTMLProps<React.ImgHTMLAttributes<HTMLImageElement>, HTMLImageElement>) {
  const { src: oldSrc, ...rest } = props
  const [loaded, setLoaded] = React.useState(false)
  const onLoad = React.useCallback(() => setLoaded(true), []);
  const src = useUnpaddedImageUrl(props.src, onLoad, true)

  React.useEffect(() => {
    setLoaded(false)
  }, [oldSrc])
  return <img {...rest} style={loaded ? undefined : hide} src={src} />
}

export const MediumIcon = styled(SearchEngineIcon)`
  width: ${icon.m};
  height: ${icon.m};
`
