import * as React from 'react'
import Fuse from 'fuse.js'

/**
 * A Hook that filters an array using the Fuse.js fuzzy-search library.
 *
 * @param list The array to filter.
 * @param searchTerm The search term to filter by.
 * @param fuseOptions Options for Fuse.js.
 *
 * @returns The filtered array.
 *
 * @see https://fusejs.io/
 */
function useFuse<T> (
    list: T[],
    searchTerm: string,
    fuseOptions?: Fuse.IFuseOptions<T>
) {
    const fuse = React.useMemo(() => {
        return new Fuse(list, fuseOptions)
    }, [list, fuseOptions])

    const results = React.useMemo(() => {
    if (searchTerm === '') {
      return list.map(value => ({
        item: Object.assign({}, value),
        matches: [],
        score: 1
      }))
    }

    return fuse.search(searchTerm)
    }, [fuse, searchTerm])

    return results.map(result => result.item)
}

export default useFuse
