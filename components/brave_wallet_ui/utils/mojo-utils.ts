// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * mojo unions must be an Object with only a single property defined.
 * This ensures that an object meets that requirement

 * @param union a mojo union or similarly structured object
 * @param unionMemberKey name of which union property to use
 * @returns mojo runtime compatible union object
 */
export const toMojoUnion = <T extends {}>(
  union: T,
  unionMemberKey: keyof T
) => {
  // typescript wants `undefined` values for other fields,
  // but mojom runtime wants no other properties assigned
  return {
    [unionMemberKey]: union[unionMemberKey]
  } as T
}
