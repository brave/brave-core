// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export const tossCoin = (weight = 0.5) => Math.random() < weight

export const project = (n: number, min: number, max: number) =>
  (max - min) * n + min

export const normal = (skew = 1) => {
  let u = 0;
    let v = 0
  while (u === 0) u = Math.random()
  while (v === 0) v = Math.random()

  let num = Math.sqrt(-2.0 * Math.log(u)) * Math.cos(2.0 * Math.PI * v)

  num = num / 10.0 + 0.5 // Translate to 0 -> 1
  if (num > 1 || num < 0) {
    num = normal(skew) // resample between 0 and 1 if out of range
  } else {
    num = Math.pow(num, skew) // Skew
  }
  return num
}

export const pickRandom = (minOrMax: number, max?: number) => {
  if (max === undefined) {
    max = minOrMax
    minOrMax = 0
  }

  return project(Math.random(), minOrMax, max)
}

export const pickRandomInt = (minOrMax: number, max?: number) =>
  Math.floor(pickRandom(minOrMax, max))

export const pickNormal = <T>(items: T[]) =>
  items.splice(Math.floor(project(normal(), 0, items.length)))[0]

export const pickUniform = <T>(items: T[]) =>
  items.splice(pickRandomInt(items.length), 1)[0]

export const pickWeighted = <T>(items: T[], getWeight: (item: T) => number) => {
  const total = items.reduce((prev, next) => prev + getWeight(next), 0)

  const value = pickRandom(total)
  let current = 0
  for (const item of items) {
    current += getWeight(item)
    if (current > value) {
      const index = items.indexOf(item)
      items.splice(index, 1)
      return item
    }
  }

  throw new Error('No items!')
}
