import json
import unittest

def GetConfigurationForBuild(defines):
    import writer_configuration
    base = writer_configuration.GetConfigurationForBuild({'_chromium'})
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
            'reg_mandatory_key_name':
                'Software\\Policies\\BraveSoftware\\Brave',
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
    # The string 'Brave' is defined in bravesoftware.adml for ADMX, but ADM
    # doesn't support external references, so we define this map here.
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
        result[k] = \
            _merge_dicts(v, dst.get(k, {})) if isinstance(v, dict) else v
    return result

class MergeDictsTest(unittest.TestCase):
    def test_merge(self):
        self.assertEqual(
            {'brave': 'Brave', "googlechrome": "Google Chrome"},
            _merge_dicts({'brave': 'Brave'}, {"googlechrome": "Google Chrome"})
        )

if __name__ == '__main__':
    unittest.main()
