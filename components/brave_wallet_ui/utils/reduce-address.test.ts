// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { reduceAddress } from './reduce-address'

describe('reduceAddress', () => {
  it('is empty for undefined', () => {
    expect(reduceAddress()).toBe('')
    expect(reduceAddress(undefined, 'x')).toBe('')
  })

  it('fill chars works', () => {
    expect(
      reduceAddress('0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14', '+++'),
    ).toBe('0x7d66+++2B14')
    expect(
      reduceAddress('bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq', '-----'),
    ).toBe('bc1qar0s-----5mdq')
  })

  it('works for Ethereium address', () => {
    expect(reduceAddress('0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14')).toBe(
      '0x7d66***2B14',
    )
  })

  it('works for Bitcoin address', () => {
    expect(reduceAddress('bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq')).toBe(
      'bc1qar0s***5mdq',
    )
    expect(reduceAddress('bc1p5d7rjq7g6rdk2yhzks9smlaqtedr4dekq08ge8')).toBe(
      'bc1p5d7r***8ge8',
    )
    expect(reduceAddress('tb1qw508d6qejxtdg4y5r3zarvary0c5xw7kxpjzsx')).toBe(
      'tb1qw508***jzsx',
    )
    expect(
      reduceAddress(
        'tb1pqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesf3hn0c',
      ),
    ).toBe('tb1pqqqq***hn0c')
  })

  it('works for Cardano address', () => {
    expect(
      reduceAddress(
        'addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wkt'
          + 'cd8cc3sq835lu7drv2xwl2wywfgse35a3x',
      ),
    ).toBe('addr1qx2f***5a3x')
    expect(
      reduceAddress(
        'addr_test1qz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqws'
          + 'x5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x',
      ),
    ).toBe('addr_test1qz2f***5a3x')
  })

  it('works for Filecoin address', () => {
    expect(reduceAddress('f1zzykebx5l6j2vxdw5px4j2jxv7z5v5lx6r6b4wi')).toBe(
      'f1zzyk***b4wi',
    )
    expect(reduceAddress('f3vvmn62lofvj2g6qkq2v4w5l6r7b4w5l6r7b4wi')).toBe(
      'f3vvmn***b4wi',
    )
    expect(reduceAddress('f410f3a6jrhk4l5p6e3y2d2q7j5z3v4q5m6q9q3v')).toBe(
      'f410f3a6***9q3v',
    )
    expect(reduceAddress('t1d2xrzcslx7l6zu5g3v3w4q5l6r7b4w5l6r7b4w')).toBe(
      't1d2xr***7b4w',
    )
    expect(reduceAddress('t3vvmn62lofvj2g6qkq2v4w5l6r7b4w5l6r7b4wi')).toBe(
      't3vvmn***b4wi',
    )
    expect(reduceAddress('t410frjq0xcm4l5p6e3y2d2q7j5z3v4q5m6q9q3v')).toBe(
      't410frjq***9q3v',
    )
  })

  it('works for ZCash address', () => {
    expect(
      reduceAddress(
        'u1z7x5l4j3k2h1g9f8d0s6a2q9w8e7r5t4y3u2i1o0p9l8k7j6h5g4f3d2s1a0q9w8e7r',
      ),
    ).toBe('u1z7x5***8e7r')
    expect(
      reduceAddress(
        'utest190jaxge023jmrqktsnae6et4pm7pmkyezzt8r674sd6a3seyrmxfry2spspl72a'
          + 'ana7kx9nfn0637np9k6tagzss48l6u9kcjf6gadlcnfusm42klsmmxnwj80q40cfw'
          + 'e8dnj7373w0',
      ),
    ).toBe('utest190ja***73w0')
    expect(reduceAddress('t1U9yhDa5XEjgfnTgZoKddeSiEN1aoLkQxq')).toBe(
      't1U9yh***kQxq',
    )
    expect(reduceAddress('t2Q4M1Q2vL3Xy5R6s7U8p9Z0aB1cD2eF3gH4jK5')).toBe(
      't2Q4M1***4jK5',
    )
    expect(reduceAddress('t3Vz22vK5z2LcKEdg16Yv4FFneEL1zg9ojd')).toBe(
      't3Vz22***9ojd',
    )
    expect(reduceAddress('tmEZhbWHTpdKMw5it8YDspUXSMGQyFwovpU')).toBe(
      'tmEZhb***ovpU',
    )
  })

  it('works for Solana address', () => {
    expect(reduceAddress('7vBjH3s8JLH5PJyJ7WGRQ2qf8X7p5QeQq6KX9J8rHqk7')).toBe(
      '7vBj***Hqk7',
    )
  })
})
