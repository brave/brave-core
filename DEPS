use_relative_paths = True

deps = {
  "vendor/ad-block": "https://github.com/brave/ad-block.git@75272a1aa83c66de6547d973f9adff776abf38ae",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@bb6013ff4d0a0191ba93158f2f3b30e7fb18c5f6",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@f86b0a5752545274e32c0dbb654c3592cc131c8a",
  "vendor/bloom-filter-cpp": "https://github.com/brave/bloom-filter-cpp.git@635780bbedff137a6a83ec23871944e22069de5b",
  "vendor/brave-extension": "https://github.com/brave/brave-extension.git@c17dadf02ad235230f7348aac09fce74cb056837",
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
