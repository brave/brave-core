// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import commander from 'commander'
import { load, write } from './lit_mangler'

commander
    .command('mangle')
    .option('-m, --mangler <file>', 'The file with containing the mangler instructions')
    .option('-i, --input <file>', 'The file to mangle')
    .option('-o, --output <file>', 'Where to output the mangled file')
    .action(async ({ mangler, input, output }: { mangler: string, input: string, output: string }) => {
        load(input)
        await import(mangler)
        write(output)
    })

commander.parse(process.argv)
