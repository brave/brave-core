import argparse
import io
import os.path
from os import walk
import sys
from lxml import etree
from lib.config import get_env_var
from lib.transifex import pull_source_files_from_transifex

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def parse_args():
  parser = argparse.ArgumentParser(description='Push strings to Transifex')
  parser.add_argument('--source_string_path',
                      nargs=1)
  return parser.parse_args()

def main():
  args = parse_args()
  source_string_path = os.path.join(SOURCE_ROOT, args.source_string_path[0])
  filename = os.path.basename(source_string_path).split('.')[0]
  extension = os.path.splitext(source_string_path)[1]
  if (extension != '.grd' and extension != '.grdp'):
    print 'returning early'
    return

  print 'Rebasing source string file:', source_string_path
  print 'filename:', filename

  content = ''
  xml_tree = etree.parse(source_string_path)
  if filename == 'extensions_resources':
    elem1 = xml_tree.xpath('//structure[@name="IDR_MD_EXTENSIONS_SIDEBAR_HTML"]')[0]
    elem1.set('preprocess', 'true')
  if filename == 'settings_resources':
    elem1 = xml_tree.xpath('//structure[@name="IDR_SETTINGS_APPEARANCE_FONTS_PAGE_HTML"]')[0]
    elem1.set('preprocess', 'true')
    elem2 = xml_tree.xpath('//structure[@name="IDR_SETTINGS_PASSWORDS_SECTION_HTML"]')[0]
    elem2.set('preprocess', 'true')
    elem3 = xml_tree.xpath('//structure[@name="IDR_SETTINGS_SITE_SETTINGS_PAGE_HTML"]')[0]
    elem3.set('preprocess', 'true')
    brave_page_visibility_element_len = len(xml_tree.xpath('//structure[@name="IDR_SETTINGS_BRAVE_PAGE_VISIBILITY_JS"]'))
    if brave_page_visibility_element_len == 0:
        brave_page_visibility_element = etree.Element('structure')
        brave_page_visibility_element.set('name', 'IDR_SETTINGS_BRAVE_PAGE_VISIBILITY_JS')
        brave_page_visibility_element.set('file', 'brave_page_visibility.js')
        brave_page_visibility_element.set('type', 'chrome_html')
        page_visibility_element = xml_tree.xpath('//structure[@name="IDR_SETTINGS_PAGE_VISIBILITY_JS"]')[0]
        page_visibility_element.addprevious(brave_page_visibility_element)

    # Add four resources for brave theme options
    # Add IDR_SETTINGS_BRAVE_APPEARANCE_BROWSER_PROXY_HTML(brave_appearance_browser_proxy.html)
    brave_appearance_browser_proxy_html_element_len = len(xml_tree.xpath('//structure[@name="IDR_SETTINGS_BRAVE_APPEARANCE_BROWSER_PROXY_HTML"]'))
    if brave_appearance_browser_proxy_html_element_len == 0:
        brave_appearance_browser_proxy_html_element = etree.Element('structure')
        brave_appearance_browser_proxy_html_element.set('name', 'IDR_SETTINGS_BRAVE_APPEARANCE_BROWSER_PROXY_HTML')
        brave_appearance_browser_proxy_html_element.set('file', 'brave_appearance_page/brave_appearance_browser_proxy.html')
        brave_appearance_browser_proxy_html_element.set('type', 'chrome_html')
        appearance_page_js_element = xml_tree.xpath('//structure[@name="IDR_SETTINGS_APPEARANCE_PAGE_JS"]')[0]
        appearance_page_js_element.addnext(brave_appearance_browser_proxy_html_element)
    # Add IDR_SETTINGS_BRAVE_APPEARANCE_BROWSER_PROXY_JS(brave_appearance_browser_proxy.js)
    brave_appearance_browser_proxy_js_element_len = len(xml_tree.xpath('//structure[@name="IDR_SETTINGS_BRAVE_APPEARANCE_BROWSER_PROXY_JS"]'))
    if brave_appearance_browser_proxy_js_element_len == 0:
        brave_appearance_browser_proxy_js_element = etree.Element('structure')
        brave_appearance_browser_proxy_js_element.set('name', 'IDR_SETTINGS_BRAVE_APPEARANCE_BROWSER_PROXY_JS')
        brave_appearance_browser_proxy_js_element.set('file', 'brave_appearance_page/brave_appearance_browser_proxy.js')
        brave_appearance_browser_proxy_js_element.set('type', 'chrome_html')
        brave_appearance_browser_proxy_js_element.set('preprocess', 'true')
        brave_appearance_browser_proxy_html_element = xml_tree.xpath('//structure[@name="IDR_SETTINGS_BRAVE_APPEARANCE_BROWSER_PROXY_HTML"]')[0]
        brave_appearance_browser_proxy_html_element.addnext(brave_appearance_browser_proxy_js_element)
    # Add IDR_SETTINGS_BRAVE_APPEARANCE_PAGE_HTML(brave_appearance_page.html)
    brave_appearance_page_html_element_len = len(xml_tree.xpath('//structure[@name="IDR_SETTINGS_BRAVE_APPEARANCE_PAGE_HTML"]'))
    if brave_appearance_page_html_element_len == 0:
        brave_appearance_page_html_element = etree.Element('structure')
        brave_appearance_page_html_element.set('name', 'IDR_SETTINGS_BRAVE_APPEARANCE_PAGE_HTML')
        brave_appearance_page_html_element.set('file', 'brave_appearance_page/brave_appearance_page.html')
        brave_appearance_page_html_element.set('type', 'chrome_html')
        brave_appearance_page_html_element.set('preprocess', 'true')
        brave_appearance_page_html_element.set('allowexternalscript', 'true')
        brave_appearance_browser_proxy_js_element = xml_tree.xpath('//structure[@name="IDR_SETTINGS_BRAVE_APPEARANCE_BROWSER_PROXY_JS"]')[0]
        brave_appearance_browser_proxy_js_element.addnext(brave_appearance_page_html_element)
    # Add IDR_SETTINGS_BRAVE_APPEARANCE_PAGE_JS(brave_appearance_page.js)
    brave_appearance_page_js_element_len = len(xml_tree.xpath('//structure[@name="IDR_SETTINGS_BRAVE_APPEARANCE_PAGE_JS"]'))
    if brave_appearance_page_js_element_len == 0:
        brave_appearance_page_js_element = etree.Element('structure')
        brave_appearance_page_js_element.set('name', 'IDR_SETTINGS_BRAVE_APPEARANCE_PAGE_JS')
        brave_appearance_page_js_element.set('file', 'brave_appearance_page/brave_appearance_page.js')
        brave_appearance_page_js_element.set('type', 'chrome_html')
        brave_appearance_page_js_element.set('preprocess', 'true')
        brave_appearance_page_html_element = xml_tree.xpath('//structure[@name="IDR_SETTINGS_BRAVE_APPEARANCE_BROWSER_PROXY_JS"]')[0]
        brave_appearance_page_html_element.addnext(brave_appearance_page_js_element)

    # Add four resources for default brave shields options
    # Add IDR_SETTINGS_BROWSER_PROXY_HTML(default_brave_shields_browser_proxy.html)
    browser_proxy_html_element_len = len(xml_tree.xpath('//structure[@name="IDR_SETTINGS_DEFAULT_BRAVE_SHIELDS_BROWSER_PROXY_HTML"]'))
    if browser_proxy_html_element_len == 0:
        browser_proxy_html_element = etree.Element('structure')
        browser_proxy_html_element.set('name', 'IDR_SETTINGS_DEFAULT_BRAVE_SHIELDS_BROWSER_PROXY_HTML')
        browser_proxy_html_element.set('file', 'default_brave_shields_page/default_brave_shields_browser_proxy.html')
        browser_proxy_html_element.set('type', 'chrome_html')
        default_brave_shields_page_js_element = xml_tree.xpath('//structure[@name="IDR_SETTINGS_APPEARANCE_PAGE_JS"]')[0]
        default_brave_shields_page_js_element.addnext(browser_proxy_html_element)
    # Add IDR_SETTINGS_BROWSER_PROXY_JS(default_brave_shields_browser_proxy.js)
    browser_proxy_js_element_len = len(xml_tree.xpath('//structure[@name="IDR_SETTINGS_DEFAULT_BRAVE_SHIELDS_BROWSER_PROXY_JS"]'))
    if browser_proxy_js_element_len == 0:
        browser_proxy_js_element = etree.Element('structure')
        browser_proxy_js_element.set('name', 'IDR_SETTINGS_DEFAULT_BRAVE_SHIELDS_BROWSER_PROXY_JS')
        browser_proxy_js_element.set('file', 'default_brave_shields_page/default_brave_shields_browser_proxy.js')
        browser_proxy_js_element.set('type', 'chrome_html')
        browser_proxy_js_element.set('preprocess', 'true')
        browser_proxy_html_element = xml_tree.xpath('//structure[@name="IDR_SETTINGS_DEFAULT_BRAVE_SHIELDS_BROWSER_PROXY_HTML"]')[0]
        browser_proxy_html_element.addnext(browser_proxy_js_element)
    # Add IDR_SETTINGS_PAGE_HTML(default_brave_shields_page.html)
    default_brave_shields_page_html_element_len = len(xml_tree.xpath('//structure[@name="IDR_SETTINGS_DEFAULT_BRAVE_SHIELDS_PAGE_HTML"]'))
    if default_brave_shields_page_html_element_len == 0:
        default_brave_shields_page_html_element = etree.Element('structure')
        default_brave_shields_page_html_element.set('name', 'IDR_SETTINGS_DEFAULT_BRAVE_SHIELDS_PAGE_HTML')
        default_brave_shields_page_html_element.set('file', 'default_brave_shields_page/default_brave_shields_page.html')
        default_brave_shields_page_html_element.set('type', 'chrome_html')
        default_brave_shields_page_html_element.set('preprocess', 'true')
        default_brave_shields_page_html_element.set('allowexternalscript', 'true')
        browser_proxy_js_element = xml_tree.xpath('//structure[@name="IDR_SETTINGS_DEFAULT_BRAVE_SHIELDS_BROWSER_PROXY_JS"]')[0]
        browser_proxy_js_element.addnext(default_brave_shields_page_html_element)
    # Add IDR_SETTINGS_PAGE_JS(default_brave_shields_page.js)
    page_js_element_len = len(xml_tree.xpath('//structure[@name="IDR_SETTINGS_DEFAULT_BRAVE_SHIELDS_PAGE_JS"]'))
    if page_js_element_len == 0:
        default_brave_shields_page_js_element = etree.Element('structure')
        default_brave_shields_page_js_element.set('name', 'IDR_SETTINGS_DEFAULT_BRAVE_SHIELDS_PAGE_JS')
        default_brave_shields_page_js_element.set('file', 'default_brave_shields_page/default_brave_shields_page.js')
        default_brave_shields_page_js_element.set('type', 'chrome_html')
        default_brave_shields_page_js_element.set('preprocess', 'true')
        default_brave_shields_page_html_element = xml_tree.xpath('//structure[@name="IDR_SETTINGS_DEFAULT_BRAVE_SHIELDS_BROWSER_PROXY_JS"]')[0]
        default_brave_shields_page_html_element.addnext(default_brave_shields_page_js_element)


  if filename == 'browser_resources':
    elem1 = xml_tree.xpath('//include[@name="IDR_MD_HISTORY_SIDE_BAR_HTML"]')[0]
    elem1.set('flattenhtml', 'true')
  if filename == 'brave_strings':
    elem1 = xml_tree.xpath('//message[@name="IDS_SXS_SHORTCUT_NAME"]')[0]
    elem1.text = 'Brave Nightly'
    elem1.attrib.pop('desc')
    elem1.attrib.pop('translateable')
    elem1 = xml_tree.xpath('//message[@name="IDS_SHORTCUT_NAME_BETA"]')[0]
    elem1.text = 'Brave Beta'
    elem1.attrib.pop('desc')
    elem1.attrib.pop('translateable')
    elem1 = xml_tree.xpath('//message[@name="IDS_SHORTCUT_NAME_DEV"]')[0]
    elem1.text = 'Brave Dev'
    elem1.attrib.pop('desc')
    elem1.attrib.pop('translateable')
    elem1 = xml_tree.xpath('//message[@name="IDS_APP_SHORTCUTS_SUBDIR_NAME_BETA"]')[0]
    elem1.text = 'Brave Apps'
    elem1.attrib.pop('desc')
    elem1.attrib.pop('translateable')
    elem1 = xml_tree.xpath('//message[@name="IDS_APP_SHORTCUTS_SUBDIR_NAME_DEV"]')[0]
    elem1.text = 'Brave Apps'
    elem1.attrib.pop('desc')
    elem1.attrib.pop('translateable')
    elem1 = xml_tree.xpath('//message[@name="IDS_INBOUND_MDNS_RULE_NAME_BETA"]')[0]
    elem1.text = 'Brave Beta (mDNS-In)'
    elem1.attrib.pop('desc')
    elem1.attrib.pop('translateable')
    elem1 = xml_tree.xpath('//message[@name="IDS_INBOUND_MDNS_RULE_NAME_CANARY"]')[0]
    elem1.text = 'Brave Nightly (mDNS-In)'
    elem1.attrib.pop('desc')
    elem1.attrib.pop('translateable')
    elem1 = xml_tree.xpath('//message[@name="IDS_INBOUND_MDNS_RULE_NAME_DEV"]')[0]
    elem1.text = 'Brave Dev (mDNS-In)'
    elem1.attrib.pop('desc')
    elem1.attrib.pop('translateable')
    elem1 = xml_tree.xpath('//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION"]')[0]
    elem1.attrib.pop('desc')
    elem1 = xml_tree.xpath('//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION_BETA"]')[0]
    elem1.text = 'Inbound rule for Brave Beta to allow mDNS traffic.'
    elem1.attrib.pop('desc')
    elem1.attrib.pop('translateable')
    elem1 = xml_tree.xpath('//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION_CANARY"]')[0]
    elem1.text = 'Inbound rule for Brave Nightly to allow mDNS traffic.'
    elem1.attrib.pop('desc')
    elem1.attrib.pop('translateable')
    elem1 = xml_tree.xpath('//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION_DEV"]')[0]
    elem1.text = 'Inbound rule for Brave Dev to allow mDNS traffic.'
    elem1.attrib.pop('desc')
    elem1.attrib.pop('translateable')
    elem1 = xml_tree.xpath('//part[@file="settings_chromium_strings.grdp"]')[0]
    elem1.set('file', 'settings_brave_strings.grdp')

  grit_root = xml_tree.xpath('//grit' if extension == '.grd' else '//grit-part')[0]
  previous_to_grit_root = grit_root.getprevious()
  comment_text = 'This file is created by l10nUtil.js. Do not edit manually.'
  if previous_to_grit_root is None or previous_to_grit_root.text != comment_text:
    comment = etree.Comment(comment_text)
    grit_root.addprevious(comment)

  transformed_content = etree.tostring(xml_tree, pretty_print=True, xml_declaration=True, encoding='UTF-8')
  # Fix some minor formatting differences from what Chromium outputs
  transformed_content = (transformed_content
    #.replace('\'', '"')
    .replace('/>', ' />'))
  print 'writing file ', source_string_path
  with open(source_string_path, mode='w') as f:
    f.write(transformed_content)
  print '-----------'

if __name__ == '__main__':
  sys.exit(main())
