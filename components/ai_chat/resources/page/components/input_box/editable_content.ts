// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Note: ContentNodes are stored in the html in the dataset of the node
// so all properties here need to be serializable as attributes.
export type ContentNode =
  | string
  | {
      type: 'skill'
      id: string
      text: string
    }

// Content of the component should be an array which can mix strings and block types.
export type Content = ContentNode[]

/**
 * Converts the content to a string. Once the mojom types are updated we
 * probably won't need to use this much.
 * @param content The content
 * @returns A stringified version of the content
 */
export const stringifyContent = (content: Content): string => {
  return content.map((c) => (typeof c === 'string' ? c : c.text)).join('')
}
