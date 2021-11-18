import xml.etree.ElementTree as ET

def _ProcessXML(root):
  # Register namespaces
  ns = { 'android' : 'http://schemas.android.com/apk/res/android' }    #NOSONAR
  for prefix, uri in ns.items():
    ET.register_namespace(prefix, uri)

  node_str = '<item xmlns:android="http://schemas.android.com/apk/res/android" '\
    'android:id="@+id/brave_wallet_id" ' \
    'android:title="@string/menu_brave_wallet" ' \
    'android:visibility="gone" />'
  node = ET.fromstring(node_str, parser=ET.XMLParser(encoding="utf-8"))

  parent = root.find('group/[@android:id="@+id/PAGE_MENU"]', namespaces=ns)
  child = parent.find('item/[@android:id="@+id/all_bookmarks_menu_id"]', namespaces=ns)
  idx = list(parent).index(child)
  parent.insert(idx + 1, node)

  return root
