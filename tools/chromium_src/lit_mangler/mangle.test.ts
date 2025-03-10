// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'fs'
import path from 'path'
import * as diff from 'diff'
import config from '../../../build/commands/lib/config.js'

// Note: Headers are stripped during preprocess, we're not interested in them
// showing up in the snapshot.
const header = `// Copyright XXXX The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

`

const root = path.resolve(__dirname, '..', '..', '..')
const chromiumSrc = path.join(root, 'chromium_src')
function* walkManglers(root=chromiumSrc) {
    for (const child of fs.readdirSync(root)) {
        const childPath = path.join(root, child)
        if (fs.statSync(childPath).isDirectory()) {
            for (const match of walkManglers(childPath)) {
                yield match
            }
        } else if (childPath.endsWith('.lit_mangler.ts')) {
            yield childPath
        }
    }
}

describe('mangled files should have up to date snapshots', () => {
    for (const mangler of walkManglers()) {
        const name = path.relative(root, mangler)
        it(`./${name} should match snapshot`, () => {
            const manglerPath = name.replace('.lit_mangler.ts', '')
                .replace('chromium_src/', '')
            const baseName = path.basename(manglerPath)

            const mangledPath = path.relative(root, path.join(config.outputDir, 'gen', manglerPath.replace(baseName, 'preprocessed/' + baseName)))
            const originalPath = path.join('..', manglerPath)

            const mangledText = fs.readFileSync(mangledPath, 'utf8')
            const originalText = fs.readFileSync(originalPath, 'utf8').substring(header.length)
            const linesDiff = diff.createTwoFilesPatch(originalPath, path.relative(config.outputDir, mangledPath), originalText, mangledText)
            expect(linesDiff).toMatchSnapshot()
        });
    }
});
