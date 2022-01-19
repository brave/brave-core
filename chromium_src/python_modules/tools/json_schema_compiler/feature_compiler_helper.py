# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

# When update this method, feature_compiler.py should be touched to make
# feature_compiler.py run. json_feature target doesn't have this in its
# dependency.
# below discard list comes from common/extensions/api/_api_features.json.
def DiscardBraveOverridesFromDupes(dupes):
    dupes.discard('topSites')
    dupes.discard('extension.inIncognitoContext')
    dupes.discard('bookmarkManagerPrivate')
    dupes.discard('bookmarks')
    dupes.discard('settingsPrivate')
    dupes.discard('sockets')
    dupes.discard('sockets.tcp')
    dupes.discard('sockets.udp')
    dupes.discard('sockets.tcpServer')
    dupes.discard('tabs')
