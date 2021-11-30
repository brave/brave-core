// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

const path = require('path')
const { URL } = require('url')
const AliasPlugin = require('enhanced-resolve/lib/AliasPlugin')
module.exports = class ChromeResourcesUriPlugin {
  apply(compiler) {
      compiler.hooks.compilation.tap(
          "ChromeResourcesUriPlugin",
          (compilation, { normalModuleFactory }) => {
            normalModuleFactory.hooks.resolveForScheme
            .for('chrome')
            .tap("ChromeResourcesUriPlugin", resourceData => {
                const url = new URL(resourceData.resource)
                resourceData.path = resourceData.resource.replace('chrome://resources', path.join(__dirname, '../../../out/Component/gen', 'ui/webui/resources/preprocessed')) + '.js'
                resourceData.query = url.search;
                resourceData.fragment = url.hash;
                resourceData.resource = resourceData.path + resourceData.query + resourceData.fragment;
                return true;
            });
          }
      );
  }
}