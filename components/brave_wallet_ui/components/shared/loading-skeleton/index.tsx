// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { LineBreak, LoadingSkeletonStyleProps, Skeleton } from './styles'

export interface LoadingSkeletonProps extends LoadingSkeletonStyleProps {
  count?: number
  wrapper?: React.FunctionComponent
}

const LoadingSkeleton = (props: LoadingSkeletonProps) => {
  const { count = 1, wrapper: Wrapper, ...styleProps } = props
  const elements: React.ReactElement[] = []
  const inline = styleProps.inline ?? false
  const enableAnimation = styleProps.enableAnimation ?? true

  for (let i = 0; i < count; i++) {
    const skeletonSpan = (
      <Skeleton
        aria-busy={enableAnimation} {...styleProps}
        enableAnimation={enableAnimation}
        key={i}
      >
        <>&zwnj;</>
      </Skeleton>
    )

    if (inline) {
      elements.push(skeletonSpan)
    } else {
      elements.push(
        <React.Fragment key={i}>
          {skeletonSpan}
          <LineBreak />
        </React.Fragment>
      )
    }
  }

  return (
    <>
      {Wrapper
        ? elements.map((el, i) => <Wrapper key={i}>{el}</Wrapper>)
        : elements
      }
    </>
  )
}

export default LoadingSkeleton
