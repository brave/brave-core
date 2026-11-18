/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export interface SectionPosition {
  route: string
  // Distance from the top of the scroll container's visible area to the top
  // of the section. Negative if the section starts above the visible area.
  top: number
}

// 'none' means the container has nothing to scroll.
export type ScrollPosition = 'none' | 'start' | 'middle' | 'end'

// A section becomes active once its top edge is within this fraction of the
// visible height, measured from the top.
const activationFraction = 0.4

// Returns the route of the section that the user is reading, or null if there
// are no sections or the page cannot scroll (so only a tab tap selects a
// section). `sections` must be in page order.
export function getActiveSectionRoute(
  sections: SectionPosition[],
  visibleHeight: number,
  scrollPosition: ScrollPosition,
): string | null {
  if (sections.length === 0 || scrollPosition === 'none') {
    return null
  }

  // In a tall window the next section can be inside the activation area even
  // when the page has not been scrolled.
  if (scrollPosition === 'start') {
    return sections[0].route
  }

  // A short last section may never reach the activation point.
  if (scrollPosition === 'end') {
    return sections[sections.length - 1].route
  }

  const threshold = visibleHeight * activationFraction
  let active = sections[0].route
  for (const section of sections) {
    if (section.top <= threshold) {
      active = section.route
    }
  }
  return active
}
