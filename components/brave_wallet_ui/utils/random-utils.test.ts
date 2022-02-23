import { randomBytes } from 'crypto'
import { unbiasedRandom } from './random-utils'

Object.defineProperty(window, 'crypto', {
    value: {
        getRandomValues: (arr: any) => randomBytes(arr.length)
    }
})

describe('unbiasedRandom()', () => {
    test('generates a random float between 0 and 1', () => {
        const float = unbiasedRandom(0, 99)
        expect(float).toBeGreaterThanOrEqual(0)
        expect(float).toBeLessThanOrEqual(99)
    })
})
