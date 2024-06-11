// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from "react";

export const defaultAllowedSchemes = ['http:', 'https:']

export type SecureLinkProps = React.DetailedHTMLProps<React.AnchorHTMLAttributes<HTMLAnchorElement>, HTMLAnchorElement> & {
  allowedSchemes?: string[]
}

export const validateScheme = (href: string | undefined, allowedSchemes: string[] = defaultAllowedSchemes) => {
  // Special case the empty url - it shouldn't crash anything
  if (!href) return

  const url = new URL(href)
  if (!allowedSchemes.includes(url.protocol)) {
    throw new Error(`Unsupported scheme ${url.protocol} on url ${href}`)
  }
}

/**
 * Endeavors to do the right thing when triggering a link via a mouse click.
 * Left click should open the link in the current tab, while
 * Control/Command/Middle click should open the link in a new tab.
 * @param href The href to open
 * @param e The mouse event
 */
export const handleOpenURLClick = (href: string | undefined, e: { ctrlKey: boolean, metaKey: boolean, buttons?: number }) => {
  validateScheme(href)

  // Control click, command click or middle click
  if (e.ctrlKey || e.metaKey || (e.buttons ?? 0) & 4) {
    window.open(href, '_blank', 'noopener noreferrer')
  } else {
    window.location.href = href!
  }
}

/**
 * A curried version of handleOpenURLClick, for use in React event handlers.
 * @param href The href the click handler should open
 * @returns A click event handler
 */
export const linkClickHandler = (href: string | undefined) => (e: React.MouseEvent) => handleOpenURLClick(href, e)

/**
 * Identical to the `a` element, except that it validates the HREF points
 * to an allowed scheme. By default, allowed schemes are http: or https:,
 * but this can be overridden.
 */
export default function SecureLink({ href, allowedSchemes, ...rest }: SecureLinkProps) {
  validateScheme(href, allowedSchemes ?? defaultAllowedSchemes)
  return <a href={href} {...rest} />
}
