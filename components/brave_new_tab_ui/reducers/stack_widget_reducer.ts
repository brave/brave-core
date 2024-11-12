/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../constants/stack_widget_types'

const widgets = {
  'rewards': 'showRewards',
  'braveTalk': 'showBraveTalk',
  'braveVPN': 'showBraveVPN'
}

const removeStackWidget = (widget: NewTab.StackWidget, state: NewTab.State): NewTab.State => {
  let { removedStackWidgets, widgetStackOrder } = state

  if (!widgetStackOrder.length) {
    return state
  }

  if (!removedStackWidgets.includes(widget)) {
    removedStackWidgets.push(widget)
  }

  state = {
    ...state,
    removedStackWidgets
  }

  return state
}

const setForegroundStackWidget = (widget: NewTab.StackWidget, state: NewTab.State): NewTab.State => {
  let newWidgetStackOrder = state.widgetStackOrder

  newWidgetStackOrder = newWidgetStackOrder.filter((stackWidget: NewTab.StackWidget) => {
    return stackWidget !== widget
  })

  newWidgetStackOrder.push(widget)

  state = {
    ...state,
    widgetStackOrder: newWidgetStackOrder
  }

  return state
}

const handleWidgetPrefsChange = (state: NewTab.State, oldState: NewTab.State): NewTab.State => {
  for (const val in widgets) {
    const widget = val as NewTab.StackWidget
    const showKey = widgets[widget]
    const newShowValue = state[showKey]
    const oldShowValue = oldState[showKey]

    if (!oldShowValue && newShowValue) {
      state = setForegroundStackWidget(widget, state)
    } else if (oldShowValue && !newShowValue) {
      state = removeStackWidget(widget, state)
    }
  }

  return state
}

const stackWidgetReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State, action) => {
  const payload = action.payload

  switch (action.type) {
    case types.SET_FOREGROUND_STACK_WIDGET:
      state = setForegroundStackWidget(payload.widget as NewTab.StackWidget, state)
      break

    default:
      break
  }

  return state
}

export {
  stackWidgetReducer,
  handleWidgetPrefsChange
}
