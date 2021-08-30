/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Notification, NotificationAction } from './notification'

export interface NotificationTitleProps {
  style?: 'funding' | 'information' | 'error'
  text: string
}

export type NotificationTitle = (props: NotificationTitleProps) => JSX.Element

export interface NotificationBodyProps {
  children: React.ReactNode
}

export type NotificationBody = (props: NotificationBodyProps) => JSX.Element

export interface NotificationActionViewProps {
  notification: Notification
  label?: string
  action?: NotificationAction
}

export type NotificationActionView =
  (props: NotificationActionViewProps) => JSX.Element

export interface NotificationViewProps {
  notification: Notification
  Title: NotificationTitle
  Body: NotificationBody
  Action: NotificationActionView
}

export type NotificationView = (props: NotificationViewProps) => JSX.Element
