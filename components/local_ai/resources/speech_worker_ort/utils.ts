// Parse a tokens.txt ("<token> <id>" per line, id-ordered, ▁ == space) into
// an id-indexed vocab array.
export function parseTokens(buf: Uint8Array): string[] {
    const vocab: string[] = []
    for (const line of new TextDecoder().decode(buf).split('\n')) {
      if (!line) {
        continue
      }
      const sp = line.lastIndexOf(' ')
      vocab[parseInt(line.slice(sp + 1), 10)] = line.slice(0, sp)
    }
    return vocab
  }
  
  // Free the model graph's JS backing store once ORT has copied it into its
  // WASM heap. (ORT-Web references its own WASM copy, so the source is no
  // longer needed.) transfer(0) detaches it immediately instead of waiting on
  // GC of the mojo-mapped shared-memory region.
  export function releaseBytes(bytes: Uint8Array): void {
    try {
      const buffer = bytes.buffer as ArrayBuffer & {
        transfer?: (newLength?: number) => ArrayBuffer
      }
      buffer.transfer?.(0)
    } catch {
      // Backing store isn't transferable; GC will reclaim it eventually.
    }
  }

  // BigBuffer is a mojo union: either inlined bytes or a shared-memory
// handle the renderer maps in.
export function readBigBuffer(
  buffer: {
    bytes?: number[]
    sharedMemory?: {
      bufferHandle: {
        mapBuffer(o: number, s: number): { buffer: ArrayBuffer | null }
      }
      size: number
    }
  },
  name: string,
): Uint8Array {
  if (buffer.bytes) {
    return new Uint8Array(buffer.bytes)
  }
  if (buffer.sharedMemory) {
    const mapped = buffer.sharedMemory.bufferHandle.mapBuffer(
      0,
      buffer.sharedMemory.size,
    )
    if (!mapped.buffer) {
      throw new Error(`Failed to map ${name} shared buffer`)
    }
    return new Uint8Array(mapped.buffer)
  }
  throw new Error(`Invalid ${name} BigBuffer`)
}
  
  // Argmax over a sub-range [start, end) of a flat array; returns the index
  // relative to `start`.
  export function argmax(a: Float32Array, start: number, end: number): number {
    let best = start
    for (let i = start + 1; i < end; i++) {
      if (a[i] > a[best]) {
        best = i
      }
    }
    return best - start
  }