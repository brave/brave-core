// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import AutoSizer from "react-virtualized-auto-sizer";
import { VariableSizeList } from "react-window";
import { isPublisherContentAllowed } from '../../../../../common/braveToday'

// Components
import { Button } from 'brave-ui'
import {
  SettingsRow,
  SettingsText
} from '../../../../components/default'
import { Toggle } from '../../../../components/toggle'
import * as Styled from './style'

interface Props {
  publishers?: BraveToday.Publishers
  setPublisherPref: (publisherId: string, enabled: boolean) => any
  onDisplay: () => any
  onClearPrefs: () => any
  showToday: boolean
  toggleShowToday: () => any
}

export const DynamicListContext = React.createContext<
  Partial<{ setSize: (index: number, size: number) => void }>
>({});

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
  const { setSize } = React.useContext(DynamicListContext);
  const rowRoot = React.useRef<null | HTMLDivElement>(null);

  React.useEffect(() => {
    if (rowRoot.current) {
      setSize && setSize(props.index, rowRoot.current.getBoundingClientRect().height);
    }
  }, [props.index, setSize, props.width]);

  const publisher = props.data[props.index]
  if (!publisher) {
    console.warn('Publisher was null at index', props.index, props.data, props)
    return null
  }
  const isChecked = isPublisherContentAllowed(publisher)

  return (
    <div style={props.style}>
      <SettingsRow innerRef={rowRoot} key={publisher.publisher_id}>
        <SettingsText>{publisher.publisher_name}</SettingsText>
        <Toggle
          checked={isChecked}
          onChange={() => props.setPublisherPref(publisher.publisher_id, !isChecked)}
          size='large'
        />
      </SettingsRow>
    </div>
  )
}

function PublisherPrefs (props: Props) {
  const listRef = React.useRef<VariableSizeList | null>(null);
  const sizeMap = React.useRef<{ [key: string]: number }>({});

  // Ensure publishers data is fetched, which won't happen
  // if user has not interacted with Brave Today on this page
  // view.
  React.useEffect(() => {
    if (props.showToday) {
      props.onDisplay()
    }
  }, [props.onDisplay, props.showToday])

  const setSize = React.useCallback((index: number, size: number) => {
    // Performance: Only update the sizeMap and reset cache if an actual value changed
    if (sizeMap.current[index] !== size) {
      sizeMap.current = { ...sizeMap.current, [index]: size };
      if (listRef.current) {
        // Clear cached data and rerender
        listRef.current.resetAfterIndex(0);
      }
    }
  }, []);

  const getSize = React.useCallback((index) => {
    return sizeMap.current[index] || 900;
  }, []);

  // Compute sorted array of publishers
  const publishers = React.useMemo(function () {
    if (props.publishers) {
      const publishers = Object.values(props.publishers)
      publishers.sort((a, b) => a.publisher_name.toLocaleLowerCase().localeCompare(b.publisher_name.toLocaleLowerCase()))
      return publishers
    }
    return null
  }, [props.publishers])

  if (!publishers) {
    return <div>Loading...</div>
  }

  return (
    <DynamicListContext.Provider
      value={{ setSize }}
    >
      <AutoSizer>
        {function ({width, height}) {
          return (
            <VariableSizeList
              ref={listRef}
              width={width}
              height={height}
              itemData={publishers}
              itemCount={publishers.length}
              itemSize={getSize}
              overscanCount={38}
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

export default function BraveTodayPrefs (props: Props) {

  const confirmAction = React.useCallback(() => {
    if (confirm('Reset all your Brave Today publisher choices to their default?')) {
      props.onClearPrefs()
    }
  }, [props.onClearPrefs])

  return (
    <Styled.Section>
      <Styled.StaticPrefs>
        <SettingsRow>
          <SettingsText>Show Brave Today?</SettingsText>
          <Toggle
            checked={props.showToday}
            onChange={props.toggleShowToday}
            size='large'
          />
        </SettingsRow>
        <SettingsRow>
          <Button type="subtle" level="tertiary" onClick={confirmAction} text="Reset Brave Today settings" />
        </SettingsRow>
      </Styled.StaticPrefs>
      {props.showToday &&
      <Styled.PublisherList>
        <PublisherPrefs {...props} />
      </Styled.PublisherList>
      }
    </Styled.Section>
  )
}