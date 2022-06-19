import * as React from 'react'

const overflowScrollableRegex = /(auto)|(scroll)/g
const isScrollable = (element: Element) => {
    return overflowScrollableRegex.test(getComputedStyle(element).overflow)
}

interface Options {
    limit: number
}

export const getScrollableParents = (element: Element | null | undefined, options?: Options) => {
    const scrollableElements: Element[] = []

    while (element && (!options?.limit || scrollableElements.length < options.limit)) {
        if (isScrollable(element)) { scrollableElements.push(element) }
        element = element.parentElement
    }

    return scrollableElements
}

export const getScrollableParent = (element: Element | null | undefined) => getScrollableParents(element, { limit: 1 })[0]

export const useParentScrolled = (element: HTMLElement | null, handler: (e: Event) => void) => {
    React.useEffect(() => {
        const scrollable = getScrollableParent(element)
        if (!scrollable) return

        scrollable.addEventListener('scroll', handler)
        return () => {
            scrollable.removeEventListener('scroll', handler)
        }
    }, [handler, element])
}
