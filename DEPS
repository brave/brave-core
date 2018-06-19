use_relative_paths = True

deps = {
  "vendor/ad-block": "https://github.com/brave/ad-block.git@11a4617c8bf3f52668e76cd4d165629294d9cc18",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@051177425a14121a22087d754ad8eb1c0ce8fb24",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@67ffffa69b56e330bab9d08f050727f891c916a1",
  "vendor/bloom-filter-cpp": "https://github.com/brave/bloom-filter-cpp.git@d511cf872ea1d650ab8dc4662f6036dac012d197",
  "vendor/brave-extension": "https://github.com/brave/brave-extension.git@38b2455adf6150dd621844f415cb5204d47c5150",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/svn2github/python-patch@a336a458016ced89aba90dfc3f4c8222ae3b1403",
  "vendor/sparkle": "https://github.com/brave/Sparkle.git@c0759cce415d7c0feae45005c8a013b1898711f0",
}

hooks = [
  {
    'name': 'bootstrap',
    'pattern': '.',
    'action': ['python', 'src/brave/script/bootstrap.py'],
  },
  {
    # Apply patches to chromium src
    'name': 'apply_patches',
    'pattern': '.',
    'action': ['python', 'src/brave/script/apply-patches.py'],
  },
  {
    # Run npm install for brave-extension
    'name': 'init',
    'pattern': '.',
    'action': ['python', 'src/brave/script/init-brave-extension.py'],
  }
]
