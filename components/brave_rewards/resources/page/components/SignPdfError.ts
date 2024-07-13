// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export const ERROR_TYPE_UNKNOWN = 1;
export const ERROR_TYPE_INPUT = 2;
export const ERROR_TYPE_PARSE = 3;
export const ERROR_VERIFY_SIGNATURE = 4;

class SignPdfError extends Error {
    static TYPE_UNKNOWN: number;
    static TYPE_INPUT: number;
    static TYPE_PARSE: number;
    static VERIFY_SIGNATURE: number;
    type: number;

    constructor(msg: any, type = ERROR_TYPE_UNKNOWN) {
        super(msg);
        this.type = type;
    }
}

// Shorthand
SignPdfError.TYPE_UNKNOWN = ERROR_TYPE_UNKNOWN;
SignPdfError.TYPE_INPUT = ERROR_TYPE_INPUT;
SignPdfError.TYPE_PARSE = ERROR_TYPE_PARSE;
SignPdfError.VERIFY_SIGNATURE = ERROR_VERIFY_SIGNATURE;

export default SignPdfError;
