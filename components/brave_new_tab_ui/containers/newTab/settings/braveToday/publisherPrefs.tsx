// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import AutoSizer from '@brave/react-virtualized-auto-sizer'
import { VariableSizeList } from 'react-window'
import { isPublisherContentAllowed } from '../../../../../common/braveToday'
import {
  SettingsRow,
  SettingsText
} from '../../../../components/default'
import { Toggle } from '../../../../components/toggle'
import * as s from './style'

export const DynamicListContext = React.createContext<
  Partial<{ setSize: (index: number, size: number) => void }>
>({})

type PublisherPrefsProps = {
  setPublisherPref: (publisherId: string, enabled: boolean) => any
  publishers: BraveToday.Publisher[]
}

type ListItemProps = {
  index: number
  width: number
  data: BraveToday.Publisher[]
  style: React.CSSProperties
  setPublisherPref: (publisherId: string, enabled: boolean) => any
}

// Component for each item. Measures height to let virtual
// list know.
function ListItem (props: ListItemProps) {
  const { setSize } = React.useContext(DynamicListContext)
  const rowRoot = React.useRef<null | HTMLDivElement>(null)

  React.useEffect(() => {
    if (rowRoot.current) {
      const marginBottom = parseFloat(getComputedStyle(rowRoot.current).marginBottom || '0')

      setSize && setSize(props.index, rowRoot.current.getBoundingClientRect().height + marginBottom)
    }
  }, [props.index, setSize, props.width])

  const publisher = props.data[props.index]
  const isChecked = publisher ? isPublisherContentAllowed(publisher) : false

  const onChange = React.useCallback(() => {
    if (!publisher) {
      return
    }
    props.setPublisherPref(publisher.publisher_id, !isChecked)
  }, [publisher, isChecked])

  if (!publisher) {
    console.warn('Publisher was null at index', props.index, props.data, props)
    return null
  }

  return (
    <s.PublisherListItem style={props.style}>
      <SettingsRow innerRef={rowRoot} key={publisher.publisher_id} onClick={onChange} isInteractive={true}>
        <SettingsText>{publisher.publisher_name}</SettingsText>
        <Toggle
          checked={isChecked}
          onChange={onChange}
          size='large'
        />
      </SettingsRow>
    </s.PublisherListItem>
  )
}

export default function PublisherPrefs (props: PublisherPrefsProps) {
  const listRef = React.useRef<VariableSizeList | null>(null)
  const sizeMap = React.useRef<{ [key: string]: number }>({})

  const setSize = React.useCallback((index: number, size: number) => {
    // Performance: Only update the sizeMap and reset cache if an actual value changed
    if (sizeMap.current[index] !== size) {
      sizeMap.current = { ...sizeMap.current, [index]: size }
      if (listRef.current) {
        // Clear cached data and rerender
        listRef.current.resetAfterIndex(0)
      }
    }
  }, [])

  const getSize = React.useCallback((index) => {
    return sizeMap.current[index] || 900
  }, [])

  const publishers = props.publishers

  return (
    <DynamicListContext.Provider
      value={{ setSize }}
    >
      <AutoSizer>
        {function ({ width, height }) {
          return (
            <VariableSizeList
              ref={listRef}
              width={width}
              height={height}
              itemData={publishers}
              itemCount={publishers.length}
              itemSize={getSize}
              overscanCount={38}
              style={{
                overscrollBehavior: 'contain'
              }}
            >
              {function (itemProps) {
                return (
                  <ListItem
                    {...itemProps}
                    width={width}
                    setPublisherPref={props.setPublisherPref}
                  />
                )
              }}
            </VariableSizeList>
          )
        }}
      </AutoSizer>
    </DynamicListContext.Provider>
  )
}
