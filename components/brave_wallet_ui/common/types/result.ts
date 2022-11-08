// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
type Result<T> = T | Error
export type Type<T> = Result<T>

export function isError<T> (result: Result<T>): result is Error {
  return result instanceof Error
}

export function isSuccess<T> (result: Result<T>): result is T {
  return !isError(result)
}
