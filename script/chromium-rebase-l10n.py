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
    parser.add_argument('--source_string_path', nargs=1)
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
    # If you modify the translateable attribute then also update
    # is_translateable_string function in brave/script/lib/transifex.py.
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
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_APP_SHORTCUTS_SUBDIR_NAME_BETA"]')[0]
        elem1.text = 'Brave Apps'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_APP_SHORTCUTS_SUBDIR_NAME_DEV"]')[0]
        elem1.text = 'Brave Apps'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_NAME_BETA"]')[0]
        elem1.text = 'Brave Beta (mDNS-In)'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_NAME_CANARY"]')[0]
        elem1.text = 'Brave Nightly (mDNS-In)'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_NAME_DEV"]')[0]
        elem1.text = 'Brave Dev (mDNS-In)'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION"]')[0]
        elem1.attrib.pop('desc')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION_BETA"]')[0]
        elem1.text = 'Inbound rule for Brave Beta to allow mDNS traffic.'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION_CANARY"]')[0]
        elem1.text = 'Inbound rule for Brave Nightly to allow mDNS traffic.'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION_DEV"]')[0]
        elem1.text = 'Inbound rule for Brave Dev to allow mDNS traffic.'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//part[@file="settings_chromium_strings.grdp"]')[0]
        elem1.set('file', 'settings_brave_strings.grdp')
        applyIsAndroidOr(xml_tree, '//part[@file="settings_brave_strings.grdp"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_ENTERPRISE_SIGNIN_TITLE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_ABOUT_BROWSER_SWITCH_DESCRIPTION_UNKNOWN_BROWSER"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_LOGIN_POD_USER_REMOVE_WARNING_SYNC"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_CONTENT_CONTEXT_ACCESSIBILITY_LABELS_BUBBLE_TEXT"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_SYNC_UPGRADE_CLIENT"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_WELCOME_HEADER"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_RELAUNCH_RECOMMENDED_TITLE"]', 2)
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_ENTERPRISE_STARTUP_CLOUD_POLICY_ENROLLMENT_TOOLTIP"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_HATS_BUBBLE_TITLE"]')

    if filename == 'extensions_strings':
        for child in xml_tree.getroot():
            if child.tag != 'outputs':
                continue
            ifElem = etree.Element('if')
            ifElem.set('expr', 'is_android')
            add_extensions_output(ifElem, 'af')
            add_extensions_output(ifElem, 'as')
            add_extensions_output(ifElem, 'az')
            add_extensions_output(ifElem, 'be')
            add_extensions_output(ifElem, 'bs')
            add_extensions_output(ifElem, 'eu')
            add_extensions_output(ifElem, 'fr-CA')
            add_extensions_output(ifElem, 'gl')
            add_extensions_output(ifElem, 'hy')
            add_extensions_output(ifElem, 'is')
            add_extensions_output(ifElem, 'ka')
            add_extensions_output(ifElem, 'kk')
            add_extensions_output(ifElem, 'km')
            add_extensions_output(ifElem, 'ky')
            add_extensions_output(ifElem, 'lo')
            add_extensions_output(ifElem, 'mk')
            add_extensions_output(ifElem, 'mn')
            add_extensions_output(ifElem, 'my')
            add_extensions_output(ifElem, 'ne')
            add_extensions_output(ifElem, 'or')
            add_extensions_output(ifElem, 'pa')
            add_extensions_output(ifElem, 'si')
            add_extensions_output(ifElem, 'sq')
            add_extensions_output(ifElem, 'sr-Latn')
            add_extensions_output(ifElem, 'ur')
            add_extensions_output(ifElem, 'uz')
            add_extensions_output(ifElem, 'zh-HK')
            add_extensions_output(ifElem, 'zu')
            child.append(ifElem)
            break

    if filename == 'ui_resources':
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_CLOSE_2"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_DEFAULT_FAVICON_32"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_NTP_DEFAULT_FAVICON"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_SIGNAL_0_BAR"]')

    if filename == 'theme_resources':
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_ACCESSIBILITY_CAPTIONS_PREVIEW_BACKGROUND"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_APP_WINDOW_CLOSE"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_BOOKMARK_BAR_APPS_SHORTCUT"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_CLOSE_1"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_DOWNLOADS_FAVICON"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_HELP_MENU"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_MANAGEMENT_FAVICON"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_PRERENDER"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_PROFILES_DICE_TURN_ON_SYNC"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_SAFETY_TIP_LOOKALIKE_ILLUSTRATION_DARK"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_SETTINGS_FAVICON"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_COOKIE_BLOCKING_ON_HEADER"]')
        applyIsAndroidOr(xml_tree, '//structure[@file="chromium/webstore_icon.png"]')
        applyIsAndroidOr(xml_tree, '//structure[@file="google_chrome/webstore_icon.png"]')

    if filename == 'browser_resources':
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_SIGNIN_SHARED_CSS_JS"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_INCOGNITO_TAB_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_DISCARDS_MOJO_API_JS"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_BROWSER_SWITCH_APP_JS"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_ABOUT_SYS_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_PAGE_NOT_AVAILABLE_FOR_GUEST_APP_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@file="resources\plugin_metadata\plugins_linux.json"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_MANAGEMENT_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_SYNC_DISABLED_CONFIRMATION_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_SIGNIN_EMAIL_CONFIRMATION_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_CONTROL_BAR_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_IDENTITY_INTERNALS_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_MEDIA_ROUTER_INTERNALS_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_TAB_RANKER_EXAMPLE_PREPROCESSOR_CONFIG_PB"]')
        elem = xml_tree.xpath('//structure[@name="IDR_INCOGNITO_TAB_HTML"]')[0]
        elem.set('file', 'resources/ntp4/incognito_tab.html')
        elem = xml_tree.xpath('//structure[@name="IDR_INCOGNITO_TAB_THEME_CSS"]')[0]
        elem.set('file', 'resources/ntp4/incognito_tab_theme.css')

    if filename == 'mojo_bindings_resources':
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_MOJO_MOJO_BINDINGS_JS"]')

    if filename == 'bookmarks_strings':
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_BOOKMARK_GROUP_FROM_FIREFOX"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_SHOW_BOOKMARK_BAR"]')

    if filename == 'generated_resources':
        applyIsAndroidOr(xml_tree, '//part[@file="settings_strings.grdp"]')
        applyIsAndroidOr(xml_tree, '//part[@file="welcome_strings.grdp"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_SEARCH_CLEARED"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_CONTENT_CONTEXT_INSPECTELEMENT"]')
        applyIsAndroidOr(xml_tree, '//message[@desc="In Title Case: The text label of a menu item for opening a new tab"]', 2)
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_DOWNLOAD_MENU_DEEP_SCAN"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_OMNIBOX_PWA_INSTALL_ICON_LABEL"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_CERT_SELECTOR_SUBJECT_COLUMN"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_TASK_MANAGER_MEM_CELL_TEXT"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_UTILITY_PROCESS_PROFILE_IMPORTER_NAME"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_PEPPER_BROKER_MESSAGE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_ABOUT_SYS_TITLE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_ABOUT_BROWSER_SWITCH_TITLE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_TOOLTIP_INTENT_PICKER_ICON"]')
        moveOnLevelHigher(xml_tree, '//message[@name="IDS_PASSWORD_MANAGER_BUBBLE_BLACKLIST_BUTTON"]')
        moveOnLevelHigher(xml_tree, '//message[@name="IDS_PASSWORD_MANAGER_TOOLTIP_SAVE"]')
        moveOnLevelHigher(xml_tree, '//message[@name="IDS_PASSWORD_MANAGER_TOOLTIP_MANAGE"]')
        moveOnLevelHigher(xml_tree, '//message[@name="IDS_PASSWORD_MANAGER_IMPORT_BUTTON"]')
        moveOnLevelHigher(xml_tree, '//message[@name="IDS_PASSWORD_MANAGER_IMPORT_DIALOG_TITLE"]')
        moveOnLevelHigher(xml_tree, '//message[@name="IDS_PASSWORD_MANAGER_EXPORT_DIALOG_TITLE"]')
        moveOnLevelHigher(xml_tree, '//message[@name="IDS_PASSWORD_MANAGER_USERNAME_LABEL"]')
        moveOnLevelHigher(xml_tree, '//message[@name="IDS_PASSWORD_MANAGER_PASSWORD_LABEL"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_IMPORT_FROM_FIREFOX"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_TOOLBAR_INFORM_SET_HOME_PAGE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_OMNIBOX_QRCODE_GENERATOR_ICON_LABEL"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_ACCNAME_INFOBAR_CONTAINER"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_MANAGED_WITH_HYPERLINK"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_APP_DEFAULT_PAGE_NAME"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_SYNC_START_SYNC_BUTTON_LABEL"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_MEDIA_GALLERIES_DIALOG_HEADER"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_BLUETOOTH_DEVICE_CHOOSER_PROMPT_ORIGIN"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_SERIAL_PORT_CHOOSER_PROMPT_ORIGIN"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_HID_CHOOSER_PROMPT_ORIGIN"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_REDIRECT_BLOCKED_MESSAGE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_DOWNLOAD_OPEN_CONFIRMATION_DIALOG_TITLE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_RELAUNCH_REQUIRED_CANCEL_BUTTON"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_WEBAUTHN_GENERIC_TITLE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_HATS_BUBBLE_TEXT"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_ENTERPRISE_EXTENSION_REQUEST_APPROVED_TITLE"]')

    if filename == 'profiles_strings':
        moveOnLevelHigher(xml_tree, '//message[@name="IDS_SYNC_USER_NAME_IN_USE_BY_ERROR"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_PROFILES_CREATE_TITLE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_USER_MANAGER_TUTORIAL_SLIDE_OUTRO_USER_NOT_FOUND"]')

    if filename == 'autofill_payments_strings':
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_AUTOFILL_FIELD_LABEL_PHONE"]')
        moveOnLevelHigher(xml_tree, '//message[@name="IDS_AUTOFILL_SAVE_CARD_BUBBLE_LOCAL_SAVE_ACCEPT"]', 2)
        moveOnLevelHigher(xml_tree, '//message[@name="IDS_AUTOFILL_SAVE_CARD_BUBBLE_UPLOAD_SAVE_ACCEPT"]', 2)
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_AUTOFILL_GOOGLE_PAY_LOGO_ACCESSIBLE_NAME"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_AUTOFILL_WEBAUTHN_OPT_IN_DIALOG_TITLE"]', 2)
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_AUTOFILL_WEBAUTHN_VERIFY_PENDING_DIALOG_TITLE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_AUTOFILL_CLOUD_TOKEN_DROPDOWN_OPTION_LABEL"]')

    if filename == 'autofill_strings':
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_AUTOFILL_POPUP_ACCESSIBLE_NODE_DATA"]')

    if filename == 'components_strings':
        applyIsAndroidOr(xml_tree, '//part[@file="management_strings.grdp"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_PRINT"]', 2)

    if filename == 'history_strings':
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_HISTORY_OTHER_SESSIONS_COLLAPSE_SESSION"]')

    if filename == 'new_or_sad_tab_strings':
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_SAD_TAB_ERROR_CODE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_NEW_TAB_OTR_HEADING"]')

    if filename == 'page_info_strings':
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_PAGE_INFO_CERTIFICATE"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_PAGE_INFO_COOKIES"]')
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_PAGE_INFO_INFOBAR_TEXT"]')

    if filename == 'payments_strings':
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_PAYMENT_REQUEST_PAYMENT_METHODS_PREVIEW"]')

    if filename == 'sync_ui_strings':
        applyIsAndroidOr(xml_tree, '//message[@name="IDS_SYNC_BASIC_ENCRYPTION_DATA"]')

    grit_root = xml_tree.xpath(
        '//grit' if extension == '.grd' else '//grit-part')[0]
    previous_to_grit_root = grit_root.getprevious()
    comment_text = 'This file is created by l10nUtil.js. Do not edit manually.'
    if previous_to_grit_root is None or (
            previous_to_grit_root.text != comment_text):
        comment = etree.Comment(comment_text)
        grit_root.addprevious(comment)

    transformed_content = etree.tostring(xml_tree, pretty_print=True,
                                         xml_declaration=True,
                                         encoding='UTF-8')
    # Fix some minor formatting differences from what Chromium outputs
    transformed_content = (transformed_content.replace('/>', ' />'))
    print 'writing file ', source_string_path
    with open(source_string_path, mode='w') as f:
        f.write(transformed_content)
    print '-----------'

def add_extensions_output(ifElem, lang):
    outputElem = etree.Element('output')
    outputElem.set('filename', 'extensions_strings_' + lang + '.pak')
    outputElem.set('type', 'data_package')
    outputElem.set('lang', lang)
    ifElem.append(outputElem)

def applyIsAndroidOr(xml_tree, toFind, parentLevel=1):
    elems = xml_tree.xpath(toFind)
    for item in elems:
        parent = item.find('..')
        if (parentLevel == 2):
            parent = parent.find('..')
        parent.set('expr', 'is_android or ' + parent.get('expr'))

def moveOnLevelHigher(xml_tree, toMove, parentLevel=1):
    elem = xml_tree.xpath(toMove)[0]
    parent = elem.find('..')
    parentOfParent = parent.find('..')
    if (parentLevel == 2):
        parentOfParent = parentOfParent.find('..')
    parentOfParent.append(elem)

if __name__ == '__main__':
    sys.exit(main())
