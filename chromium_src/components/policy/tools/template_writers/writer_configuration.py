# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils

@override_utils.override_function(globals())
def GetConfigurationForBuild(original_function, defines):
  base = original_function(defines)
  return _merge_dicts(_BRAVE_VALUES, base)

_BRAVE_VALUES = {
  'build': 'brave',
  'app_name': 'Brave',
  'doc_url':
    'https://support.brave.com/hc/en-us/articles/360039248271-Group-Policy',
  'frame_name': 'Brave Frame',
  'webview_name': 'Brave WebView',
  'win_config': {
    'win': {
      'reg_mandatory_key_name': 'Software\\Policies\\BraveSoftware\\Brave',
      'reg_recommended_key_name':
        'Software\\Policies\\BraveSoftware\\Brave\\Recommended',
      'mandatory_category_path': ['Brave:Cat_Brave', 'brave'],
      'recommended_category_path': ['Brave:Cat_Brave', 'brave_recommended'],
      'category_path_strings': {
        'brave': 'Brave',
        'brave_recommended':
        'Brave - {doc_recommended}'
      },
      'namespace': 'BraveSoftware.Policies.Brave',
    },
  },
  # The string 'Brave' is defined in brave.adml for ADMX, but ADM doesn't
  # support external references, so we define this map here.
  'adm_category_path_strings': {
    'Brave:Cat_Brave': 'Brave'
  },
  'admx_prefix': 'brave',
  'admx_using_namespaces': {
    'Brave': 'BraveSoftware.Policies'  # prefix: namespace
  },
  'linux_policy_path': '/etc/brave/policies/',
  'bundle_id': 'com.brave.ios.core',
}

def _merge_dicts(src, dst):
  result = dict(dst)
  for k, v in src.items():
    result[k] = _merge_dicts(v, dst.get(k, {})) if isinstance(v, dict) else v
  return result
