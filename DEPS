use_relative_paths = True

deps = {
  "vendor/ad-block": "https://github.com/brave/ad-block.git@9b1ff3275a2f4ce76ad3aaa749e4a01f32a9dabf",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@0931529eba33109c6b3946a83295577fea540045",
  "vendor/hashset-cpp": "https://github.com/bbondy/hashset-cpp.git@f427324d667d7188a9e0975cca7f3a8c06226b4d",
  "vendor/bloom-filter-cpp": "https://github.com/bbondy/bloom-filter-cpp.git@6faa14ececa33badad149c40f94ff9867159681c",
  "vendor/brave-extension": "https://github.com/brave/brave-extension.git@64aa807bcf16effabc4a37cfed696e077a6c1974",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/svn2github/python-patch@a336a458016ced89aba90dfc3f4c8222ae3b1403",
}

hooks = [
  {
    # Apply patches to chromium src
    'name': 'apply_patches',
    'pattern': '.',
    'action': ['python', 'src/brave/script/apply-patches.py'],
  }
]
