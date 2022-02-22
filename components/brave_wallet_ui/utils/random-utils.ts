export function unbiasedRandom(min: number, max: number) {
    const range = max - min + 1
    const bytes_needed = Math.ceil(Math.log2(range) / 8)
    const cutoff = Math.floor((256 ** bytes_needed) / range) * range
    const bytes = new Uint8Array(bytes_needed)
    let value
    do {
        window.crypto.getRandomValues(bytes)
        value = bytes.reduce((acc, x, n) => acc + x * 256 ** n, 0)
    } while (value >= cutoff)
    return min + value % range
}
