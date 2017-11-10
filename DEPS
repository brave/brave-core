use_relative_paths = True

deps = {
  "vendor/ad-block": "https://github.com/brave/ad-block.git@b74f508be52931604199195905d3113bf450c851",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@f4e56becfc197b76cbb726b6813afe1c9dd55b88",
  "vendor/hashset-cpp": "https://github.com/bbondy/hashset-cpp.git@728fd67bc269765f5a566fb1d2fd9b04b632e68a",
  "vendor/bloom-filter-cpp": "https://github.com/bbondy/bloom-filter-cpp.git@b5509def04d1ecf60fdad62457a3bd09c457df90",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/svn2github/python-patch@a336a458016ced89aba90dfc3f4c8222ae3b1403",
}

hooks = [
  {
    # Apply patches to chromium src
    'name': 'apply_patches',
    'pattern': '.',
    'action': ['python', 'src/electron/script/apply-patches.py'],
  }
]
