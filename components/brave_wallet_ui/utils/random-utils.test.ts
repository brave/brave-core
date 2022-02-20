import { randomFloat } from './random-utils';

describe('randomFloat()', () => {
    test('generates a random float between 0 and 1', () => {
        const float = randomFloat()
        expect(float).toBeGreaterThanOrEqual(0)
        expect(float).toBeLessThanOrEqual(1)
    })
})