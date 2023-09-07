# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import xml.etree.ElementTree as ET

def _ProcessXML(root):
    # Register namespaces
    ns = {
        'android': 'http://schemas.android.com/apk/res/android',  #NOSONAR
        'app': 'http://schemas.android.com/apk/res-auto'  #NOSONAR
    }
    for prefix, uri in ns.items():
        ET.register_namespace(prefix, uri)

    node_str = '<item xmlns:android='\
    '"http://schemas.android.com/apk/res/android" '\
      'android:id="@+id/brave_wallet_id" ' \
      'android:title="@string/menu_brave_wallet" ' \
      'android:visibility="gone" />'
    node = ET.fromstring(node_str, parser=ET.XMLParser(encoding="utf-8"))

    parent = root.find('group/[@android:id="@+id/PAGE_MENU"]', namespaces=ns)
    child = parent.find('item/[@android:id="@+id/all_bookmarks_menu_id"]',
                        namespaces=ns)
    idx = list(parent).index(child)
    parent.insert(idx + 1, node)

    playlist_node_str = '<item xmlns:android='\
    '"http://schemas.android.com/apk/res/android" '\
      'android:id="@+id/brave_playlist_id" ' \
      'android:title="@string/brave_playlist" />'
    playlist_node = ET.fromstring(playlist_node_str,
                                  parser=ET.XMLParser(encoding="utf-8"))

    parent = root.find('group/[@android:id="@+id/PAGE_MENU"]', namespaces=ns)
    child = parent.find('item/[@android:id="@+id/brave_wallet_id"]',
                        namespaces=ns)
    idx = list(parent).index(child)
    parent.insert(idx + 1, playlist_node)

    brave_leo_node_str = '<item xmlns:android='\
        '"http://schemas.android.com/apk/res/android" '\
          'android:id="@+id/brave_leo_id" ' \
          'android:title="@string/menu_brave_leo" />'
    brave_leo_node = ET.fromstring(brave_leo_node_str,
                                   parser=ET.XMLParser(encoding="utf-8"))

    parent = root.find('group/[@android:id="@+id/PAGE_MENU"]', namespaces=ns)
    child = parent.find('item/[@android:id="@+id/brave_playlist_id"]',
                        namespaces=ns)
    idx = list(parent).index(child)
    parent.insert(idx + 1, brave_leo_node)

    set_as_default_node_str = '<item xmlns:android='\
    '"http://schemas.android.com/apk/res/android" '\
      'android:id="@+id/set_default_browser" ' \
      'android:title="@string/menu_set_default_browser" />'
    set_as_default_node = ET.fromstring(set_as_default_node_str,
                                        parser=ET.XMLParser(encoding="utf-8"))
    set_as_default_child = parent.find(
        'item/[@android:id="@+id/preferences_id"]', namespaces=ns)
    set_as_default_idx = list(parent).index(set_as_default_child)
    parent.insert(set_as_default_idx + 1, set_as_default_node)

    vpn_node_str = '<item xmlns:android='\
    '"http://schemas.android.com/apk/res/android" '\
            'android:id="@+id/request_brave_vpn_row_menu_id" '\
            'android:title="@null"> '\
            '<menu> '\
                '<item android:id="@+id/request_brave_vpn_id" '\
                  'android:title="@string/brave_vpn" '\
                  'android:icon="@drawable/ic_vpn" /> '\
                '<item android:id="@+id/request_brave_vpn_check_id" '\
                  'android:title="@null" '\
                  'android:checkable="true" /> '\
            '</menu> '\
            '</item>'
    vpn_node = ET.fromstring(vpn_node_str,
                             parser=ET.XMLParser(encoding="utf-8"))
    vpn_child = parent.find('item/[@android:id="@+id/set_default_browser"]',
                            namespaces=ns)
    vpn_idx = list(parent).index(vpn_child)
    parent.insert(vpn_idx + 1, vpn_node)

    speedreader_node_str = '<item xmlns:android='\
            '"http://schemas.android.com/apk/res/android" '\
            'android:id="@+id/brave_speedreader_id" '\
            'android:title="@string/brave_speedreader_title" '\
            'android:icon="@drawable/ic_readermode"/>'
    speedreader_node = ET.fromstring(speedreader_node_str,
                                     parser=ET.XMLParser(encoding="utf-8"))
    speedreader_child = parent.find('item/[@android:id="@+id/page_zoom_id"]',
                                    namespaces=ns)
    speedreader_idx = list(parent).index(speedreader_child)
    parent.insert(speedreader_idx, speedreader_node)

    return root
