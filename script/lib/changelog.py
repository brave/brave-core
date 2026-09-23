import requests
import mistletoe

from mistletoe import Document, ast_renderer


def render_markdown(changelog_txt, version, logging):
    """
    Create an AST from the changelog document (which is in Markdown).
    Search the AST for the version we're interested in, generate
    and return markdown as a string for just that version.
    """

    d = Document(changelog_txt)
    output = ast_renderer.get_ast(d)
    s = str()

    logging.debug(
        "Locating the changelog AST for version: \'{}\'".format(version))

    version_heading = ''
    version_changes = ''
    pos = 0
    for item in output['children']:
        if item['type'] == 'Heading' and item['level'] == 2:
            if item['children'][0]['children'][0]['content'] == version:
                version_heading = output['children'][pos]
                version_changes = output['children'][pos+1]
        pos = pos + 1
    if version_heading and version_changes:
        heading = '# Release Notes'
        s = heading + '\n'
        s = s + '\n'
        changes = reconstruct_brave_changelog_list(version_changes)
        for i in changes:
            s = s + i + '\n'
    else:
        logging.error(
            "Cannot Locate the changelog AST for version: \'{}\'".format(version))
        exit(1)

    return s


def render_html(changelog_txt, version, logging):
    """
    Format an html rendered document from the render_markdown method above.
    """

    rendered = mistletoe.markdown(
        render_markdown(changelog_txt, version, logging))
    return rendered


def reconstruct_brave_changelog_list(li):
    """
    li is a list
    """

    changes = []

    for item in li['children']:
        # For special markdown characters such as *EMPHASIS* or `inline code
        # blocks`, mistletoe's AST provides us with a nested list. We need
        # to traverse the list elements and append them to a single string.
        # During this process we also remove the special characters.
        appended_entry = str()
        for item2 in item['children'][0]['children']:
            if 'RawText' in item2['type']:
                appended_entry = appended_entry + '{}'.format(item2['content'])
            elif 'Link' in item2['type']:
                appended_entry = appended_entry + \
                    '[{}]({})'.format(item2['children']
                                      [0]['content'], item2['target'])
            elif 'InlineCode' in item2['type']:
                appended_entry = appended_entry + \
                    '{}'.format(item2['children'][0]['content'])
            else:
                # Try to catch all other markdown types in this general case
                #
                # Mistletoe types defined here:
                # https://github.com/miyuchina/mistletoe/blob/master/mistletoe/base_renderer.py#L47-L71
                appended_entry = appended_entry + \
                    '{}'.format(item2['children'][0]['content'])

        changes.append(" {} {}".format(' -', appended_entry))
    return changes


def download_from_url(args, logging, changelog_url):
    headers = {'Accept': 'application/octet-stream'}
    if args.debug:
        # disable urllib3 logging for this session to avoid showing
        # access_token in logs
        logging.getLogger("urllib3").setLevel(logging.WARNING)
    logging.info("Downloading Changelog file: {}".format(changelog_url))
    try:
        r = requests.get(changelog_url, headers=headers)
    except requests.exceptions.ConnectionError as e:
        logging.error(
            "Error: Received requests.exceptions.ConnectionError, Exiting...")
        exit(1)
    except Exception as e:
        logging.error(
            "Error: Received exception {},  Exiting...".format(type(e)))
        exit(1)
    logging.debug("r.status_code: {}".format(r.status_code))

    if r.status_code == 200:
        return r.text
    else:
        r.raise_for_status()
