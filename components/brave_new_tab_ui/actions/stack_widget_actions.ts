// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/stack_widget_types'

export const setForegroundStackWidget = (widget: NewTab.StackWidget) => action(types.SET_FOREGROUND_STACK_WIDGET, {
  widget
})

export const saveWidgetStackOrder = () => action(types.SAVE_WIDGET_STACK_ORDER)
