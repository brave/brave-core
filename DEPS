use_relative_paths = True

deps = {
  "vendor/ad-block": "https://github.com/brave/ad-block.git@9b1ff3275a2f4ce76ad3aaa749e4a01f32a9dabf",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@0931529eba33109c6b3946a83295577fea540045",
  "vendor/hashset-cpp": "https://github.com/bbondy/hashset-cpp.git@f427324d667d7188a9e0975cca7f3a8c06226b4d",
  "vendor/bloom-filter-cpp": "https://github.com/bbondy/bloom-filter-cpp.git@6faa14ececa33badad149c40f94ff9867159681c",
  "vendor/brave-extension": "https://github.com/brave/brave-extension.git@a632d49d21353790652166066a23876c9f27084e",
  "vendor/site-hacks-extension": "https://github.com/brave/site-hacks-extension.git@8aca8b3969b50b9c739a326e89c05d3fa7e0df00",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/svn2github/python-patch@a336a458016ced89aba90dfc3f4c8222ae3b1403",
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
