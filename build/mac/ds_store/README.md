The DS_Store files in this directory configure the background image and shortcut
positions in Brave's DMG archives.

The files are checked into version control but can be regenerated by running the
`update_ds_store_files.py` script in this directory. This requires the
`ds_store` Python library. The script was tested with Python 3.12.8, `ds_store`
1.3.1 and `mac_alias` 2.2.2.
