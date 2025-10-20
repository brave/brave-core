use std::time::Duration;

use chrono::{DateTime, Utc};
use mime::Mime;

use crate::parser::util;
#[cfg(test)]
use crate::parser::util::parse_timestamp_lenient;
use url::Url;

/// Combined model for a syndication feed (i.e. RSS1, RSS 2, Atom, JSON Feed)
///
/// The model is based on the Atom standard as a start with RSS1+2 mapped on to it e.g.
/// * Atom
///     * Feed -> Feed
///     * Entry -> Entry
/// * RSS 1 + 2
///     * Channel -> Feed
///     * Item -> Entry
///
/// [Atom spec]: http://www.atomenabled.org/developers/syndication/
/// [RSS 2 spec]: https://validator.w3.org/feed/docs/rss2.html
/// [RSS 1 spec]: https://validator.w3.org/feed/docs/rss1.html
/// [MediaRSS spec]: https://www.rssboard.org/media-rss
/// [iTunes podcast spec]: https://help.apple.com/itc/podcasts_connect/#/itcb54353390
/// [iTunes podcast guide]: https://www.feedforall.com/itune-tutorial-tags.htm
///
/// Certain elements are not mapped given their limited utility:
///   * RSS 2:
///     * channel - docs (pointer to the spec), cloud (for callbacks), textInput (text box e.g. for search)
///     * item - comments (link to comments on the article), source (pointer to the channel, but our data model links items to a channel)
///   * RSS 1:
///     * channel - rdf:about attribute (pointer to feed), textinput (text box e.g. for search)
#[derive(Clone, Debug, PartialEq)]
pub struct Feed {
    /// Type of this feed (e.g. RSS2, Atom etc)
    pub feed_type: FeedType,
    /// A unique identifier for this feed
    /// * Atom (required): Identifies the feed using a universally unique and permanent URI.
    /// * RSS doesn't require an ID so it is initialised to the hash of the first link or a UUID if not found
    pub id: String,
    /// The title of the feed
    /// * Atom (required): Contains a human readable title for the feed. Often the same as the title of the associated website. This value should not be blank.
    /// * RSS 1 + 2 (required) "title": The name of the channel. It's how people refer to your service.
    /// * JSON Feed: is the name of the feed
    pub title: Option<Text>,
    /// The time at which the feed was last modified. If not provided in the source, or invalid, it is `None`.
    /// * Atom (required): Indicates the last time the feed was modified in a significant way.
    /// * RSS 2 (optional) "lastBuildDate": The last time the content of the channel changed.
    pub updated: Option<DateTime<Utc>>,

    /// Atom (recommended): Collection of authors defined at the feed level.
    /// JSON Feed: specifies the feed author.
    pub authors: Vec<Person>,
    /// Description of the feed
    /// * Atom (optional): Contains a human-readable description or subtitle for the feed (from <subtitle>).
    /// * RSS 1 + 2 (required): Phrase or sentence describing the channel.
    /// * JSON Feed: description of the feed
    pub description: Option<Text>,
    /// Links to related pages
    /// * Atom (recommended): Identifies a related Web page.
    /// * RSS 1 + 2 (required): The URL to the HTML website corresponding to the channel.
    /// * JSON Feed: the homepage and feed URLs
    pub links: Vec<Link>,

    /// Structured classification of the feed
    /// * Atom (optional): Specifies a category that the feed belongs to. A feed may have multiple category elements.
    /// * RSS 2 (optional) "category": Specify one or more categories that the channel belongs to.
    pub categories: Vec<Category>,
    /// People who have contributed to the feed
    /// * Atom (optional): Names one contributor to the feed. A feed may have multiple contributor elements.
    /// * RSS 2 (optional) "managingEditor": Email address for person responsible for editorial content.
    /// * RSS 2 (optional) "webMaster": Email address for person responsible for technical issues relating to channel.
    pub contributors: Vec<Person>,
    /// Information on the software used to build the feed
    /// * Atom (optional): Identifies the software used to generate the feed, for debugging and other purposes.
    /// * RSS 2 (optional): A string indicating the program used to generate the channel.
    pub generator: Option<Generator>,
    /// A small icon
    /// * Atom (optional): Identifies a small image which provides iconic visual identification for the feed.
    /// * JSON Feed: is the URL of an image for the feed suitable to be used in a source list.
    pub icon: Option<Image>,
    /// RSS 2 (optional): The language the channel is written in.
    pub language: Option<String>,
    /// An image used to visually identify the feed
    /// * Atom (optional): Identifies a larger image which provides visual identification for the feed.
    /// * RSS 1 + 2 (optional) "image": Specifies a GIF, JPEG or PNG image that can be displayed with the channel.
    /// * JSON Feed: is the URL of an image for the feed suitable to be used in a timeline
    pub logo: Option<Image>,
    /// RSS 2 (optional): The publication date for the content in the channel.
    pub published: Option<DateTime<Utc>>,
    /// Rating for the content
    /// * Populated from the media or itunes namespaces
    pub rating: Option<MediaRating>,
    /// Rights restricting content within the feed
    /// * Atom (optional): Conveys information about rights, e.g. copyrights, held in and over the feed.
    /// * RSS 2 (optional) "copyright": Copyright notice for content in the channel.
    pub rights: Option<Text>,
    /// RSS 2 (optional): It's a number of minutes that indicates how long a channel can be cached before refreshing from the source.
    pub ttl: Option<u32>,

    /// The individual items within the feed
    /// * Atom (optional): Individual entries within the feed (e.g. a blog post)
    /// * RSS 1+2 (optional): Individual items within the channel.
    pub entries: Vec<Entry>,
}

impl Feed {
    pub(crate) fn new(feed_type: FeedType) -> Self {
        Feed {
            feed_type,
            id: "".into(),
            title: None,
            updated: None,
            authors: Vec::new(),
            description: None,
            links: Vec::new(),
            categories: Vec::new(),
            contributors: Vec::new(),
            generator: None,
            icon: None,
            language: None,
            logo: None,
            published: None,
            rating: None,
            rights: None,
            ttl: None,
            entries: Vec::new(),
        }
    }
}

#[cfg(test)]
impl Feed {
    pub fn author(mut self, person: Person) -> Self {
        self.authors.push(person);
        self
    }

    pub fn category(mut self, category: Category) -> Self {
        self.categories.push(category);
        self
    }

    pub fn contributor(mut self, person: Person) -> Self {
        self.contributors.push(person);
        self
    }

    pub fn description(mut self, description: Text) -> Self {
        self.description = Some(description);
        self
    }

    pub fn entry(mut self, entry: Entry) -> Self {
        self.entries.push(entry);
        self
    }

    pub fn generator(mut self, generator: Generator) -> Self {
        self.generator = Some(generator);
        self
    }

    pub fn icon(mut self, image: Image) -> Self {
        self.icon = Some(image);
        self
    }

    pub fn id(mut self, id: &str) -> Self {
        self.id = id.to_string();
        self
    }

    pub fn language(mut self, language: &str) -> Self {
        self.language = Some(language.to_owned());
        self
    }

    pub fn link(mut self, link: Link) -> Self {
        self.links.push(link);
        self
    }

    pub fn logo(mut self, image: Image) -> Self {
        self.logo = Some(image);
        self
    }

    pub fn published(mut self, pub_date: &str) -> Self {
        self.published = parse_timestamp_lenient(pub_date);
        self
    }

    pub fn rights(mut self, rights: Text) -> Self {
        self.rights = Some(rights);
        self
    }

    pub fn title(mut self, title: Text) -> Self {
        self.title = Some(title);
        self
    }

    pub fn ttl(mut self, ttl: u32) -> Self {
        self.ttl = Some(ttl);
        self
    }

    pub fn updated(mut self, updated: Option<DateTime<Utc>>) -> Self {
        self.updated = updated;
        self
    }

    pub fn updated_parsed(mut self, updated: &str) -> Self {
        self.updated = parse_timestamp_lenient(updated);
        self
    }
}

/// Type of a feed (RSS, Atom etc)
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum FeedType {
    Atom,
    JSON,
    RSS0,
    RSS1,
    RSS2,
}

/// An item within a feed
#[derive(Clone, Debug, PartialEq)]
pub struct Entry {
    /// A unique identifier for this item with a feed. If not supplied it is initialised to a hash of the first link or a UUID if not available.
    /// * Atom (required): Identifies the entry using a universally unique and permanent URI.
    /// * RSS 2 (optional) "guid": A string that uniquely identifies the item.
    /// * RSS 1: does not specify a unique ID as a separate item, but does suggest the URI should be "the same as the link" so we use a hash of the link if found
    /// * JSON Feed: is unique for that item for that feed over time.
    pub id: String,
    /// Title of this item within the feed
    /// * Atom, RSS 1(required): Contains a human readable title for the entry.
    /// * RSS 2 (optional): The title of the item.
    /// * JSON Feed: The title of the item.
    pub title: Option<Text>,
    /// Time at which this item was last modified. If not provided in the source, or invalid, it is `None`.
    /// * Atom (required): Indicates the last time the entry was modified in a significant way.
    /// * RSS doesn't specify this field.
    /// * JSON Feed: the last modification date of this item
    pub updated: Option<DateTime<Utc>>,

    /// Authors of this item
    /// * Atom (recommended): Collection of authors defined at the entry level.
    /// * RSS 2 (optional): Email address of the author of the item.
    /// * JSON Feed: the author of the item
    pub authors: Vec<Person>,
    /// The content of the item
    /// * Atom (recommended): Contains or links to the complete content of the entry.
    /// * RSS 2 (optional) "content:encoded": The HTML form of the content
    /// * JSON Feed: the html content of the item, or the text content if no html is specified
    pub content: Option<Content>,
    /// Links associated with this item
    /// * Atom (recommended): Identifies a related Web page.
    /// * RSS 2 (optional): The URL of the item.
    /// * RSS 1 (required): The item's URL.
    /// * JSON Feed: the url and external URL for the item is the first items, then each subsequent attachment
    pub links: Vec<Link>,
    /// A short summary of the item
    /// * Atom (recommended): Conveys a short summary, abstract, or excerpt of the entry.
    /// * RSS 1 (optional): Populated from the RSS namespace 'description' field, or if not present, the Dublin Core namespace 'description' field.
    /// * RSS 2 (optional): Populated from the RSS namespace 'description' field.
    /// * JSON Feed: the summary for the item, or the text content if no summary is provided and both text and html content are specified
    ///
    /// Warning: Some feeds (especially RSS) use significant whitespace in this field even in cases where it should be considered HTML. Consider rendering this field in a way that preserves whitespace-based formatting such as a double-newline to separate paragraphs.
    pub summary: Option<Text>,

    /// Structured classification of the item
    /// * Atom (optional): Specifies a category that the entry belongs to. A feed may have multiple category elements.
    /// * RSS 2 (optional): Includes the item in one or more categories.
    /// * JSON Feed: the supplied item tags
    pub categories: Vec<Category>,
    /// Atom (optional): Names one contributor to the entry. A feed may have multiple contributor elements.
    pub contributors: Vec<Person>,
    /// Time at which this item was first published
    /// * Atom (optional): Contains the time of the initial creation or first availability of the entry.
    /// * RSS 2 (optional) "pubDate": Indicates when the item was published.
    /// * JSON Feed: the date at which the item was published
    pub published: Option<DateTime<Utc>>,
    /// Atom (optional): If an entry is copied from one feed into another feed, then this contains the source feed metadata.
    pub source: Option<String>,
    /// Atom (optional): Conveys information about rights, e.g. copyrights, held in and over the feed.
    pub rights: Option<Text>,

    /// Extension for MediaRSS - https://www.rssboard.org/media-rss
    /// A MediaObject will be created in two cases:
    /// 1) each "media:group" element encountered in the feed
    /// 2) a default for any other "media:*" elements found at the item level
    /// See the Atom tests for youtube and newscred for examples
    pub media: Vec<MediaObject>,

    /// Atom (optional): The language specified on the item
    pub language: Option<String>,
    /// Atom (optional): The base url specified on the item to resolve any relative
    /// references found within the scope on the item
    pub base: Option<String>,
}

impl Default for Entry {
    fn default() -> Self {
        Entry {
            id: "".into(),
            title: None,
            updated: None,
            authors: Vec::new(),
            content: None,
            links: Vec::new(),
            summary: None,
            categories: Vec::new(),
            contributors: Vec::new(),
            published: None,
            source: None,
            rights: None,
            media: Vec::new(),
            language: None,
            base: None,
        }
    }
}

#[cfg(test)]
impl Entry {
    pub fn author(mut self, person: Person) -> Self {
        self.authors.push(person);
        self
    }

    pub fn category(mut self, category: Category) -> Self {
        self.categories.push(category);
        self
    }

    pub fn content(mut self, content: Content) -> Self {
        self.content = Some(content);
        self
    }

    pub fn contributor(mut self, person: Person) -> Self {
        self.contributors.push(person);
        self
    }

    pub fn id(mut self, id: &str) -> Self {
        self.id = id.trim().to_string();
        self
    }

    pub fn link(mut self, link: Link) -> Self {
        self.links.push(link);
        self
    }

    pub fn published(mut self, published: &str) -> Self {
        self.published = parse_timestamp_lenient(published);
        self
    }

    pub fn rights(mut self, rights: Text) -> Self {
        self.rights = Some(rights);
        self
    }

    pub fn summary(mut self, summary: Text) -> Self {
        self.summary = Some(summary);
        self
    }

    pub fn title(mut self, title: Text) -> Self {
        self.title = Some(title);
        self
    }

    pub fn updated(mut self, updated: Option<DateTime<Utc>>) -> Self {
        self.updated = updated;
        self
    }

    pub fn updated_parsed(mut self, updated: &str) -> Self {
        self.updated = parse_timestamp_lenient(updated);
        self
    }

    pub fn media(mut self, media: MediaObject) -> Self {
        self.media.push(media);
        self
    }

    pub fn language(mut self, language: &str) -> Self {
        self.language = Some(language.to_owned());
        self
    }

    pub fn base(mut self, url: &str) -> Self {
        self.base = Some(url.to_owned());
        self
    }
}

/// Represents the category of a feed or entry
///
/// [Atom spec]: http://www.atomenabled.org/developers/syndication/#category
/// [RSS 2 spec]: https://validator.w3.org/feed/docs/rss2.html#ltcategorygtSubelementOfLtitemgt
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Category {
    /// The category as a human readable string
    /// * Atom (required): Identifies the category.
    /// * RSS 2: The value of the element is a forward-slash-separated string that identifies a hierarchic location in the indicated taxonomy. Processors may establish conventions for the interpretation of categories.
    /// * JSON Feed: the value of the tag
    pub term: String,
    /// Atom (optional): Identifies the categorization scheme via a URI.
    pub scheme: Option<String>,
    /// Atom (optional): Provides a human-readable label for display.
    pub label: Option<String>,
    /// Sub-categories (typically from the iTunes namespace i.e. https://help.apple.com/itc/podcasts_connect/#/itcb54353390)
    pub subcategories: Vec<Category>,
}

impl Category {
    pub fn new(term: &str) -> Category {
        Category {
            term: term.trim().into(),
            scheme: None,
            label: None,
            subcategories: Vec::new(),
        }
    }
}

#[cfg(test)]
impl Category {
    pub fn label(mut self, label: &str) -> Self {
        self.label = Some(label.to_owned());
        self
    }

    pub fn scheme(mut self, scheme: &str) -> Self {
        self.scheme = Some(scheme.to_owned());
        self
    }
}

/// Content, or link to the content, for a given entry.
///
/// [Atom spec]: http://www.atomenabled.org/developers/syndication/#contentElement
/// [RSS 2.0]: https://validator.w3.org/feed/docs/rss2.html#ltenclosuregtSubelementOfLtitemgt
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Content {
    /// Atom
    /// * If the type attribute ends in +xml or /xml, then an xml document of this type is contained inline.
    /// * If the type attribute starts with text, then an escaped document of this type is contained inline.
    /// * Otherwise a base64 encoded document of the indicated media type is contained inline.
    pub body: Option<String>,
    /// Type of content
    /// * Atom: The type attribute is either text, html, xhtml, in which case the content element is defined identically to other text constructs.
    /// * RSS 2: Type says what its type is, a standard MIME type
    pub content_type: Mime,
    /// RSS 2.0: Length of the content in bytes
    pub length: Option<u64>,
    /// Source of the content
    /// * Atom: If the src attribute is present, it represents the URI of where the content can be found. The type attribute, if present, is the media type of the content.
    /// * RSS 2.0: where the enclosure is located
    pub src: Option<Link>,
}

impl Default for Content {
    fn default() -> Content {
        Content {
            body: None,
            content_type: mime::TEXT_PLAIN,
            length: None,
            src: None,
        }
    }
}

#[cfg(test)]
impl Content {
    pub fn body(mut self, body: &str) -> Self {
        self.body = Some(body.to_owned());
        self
    }

    pub fn content_type(mut self, content_type: &str) -> Self {
        self.content_type = content_type.parse::<Mime>().unwrap();
        self
    }

    pub fn length(mut self, length: u64) -> Self {
        self.length = Some(length);
        self
    }

    pub fn src(mut self, url: &str) -> Self {
        self.src = Some(Link::new(url, None));
        self
    }
}

/// Information on the tools used to generate the feed
///
/// Atom: Identifies the software used to generate the feed, for debugging and other purposes.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Generator {
    /// Atom: Additional data
    /// RSS 2: A string indicating the program used to generate the channel.
    pub content: String,
    /// Atom: Link to the tool
    pub uri: Option<String>,
    /// Atom: Tool version
    pub version: Option<String>,
}

impl Generator {
    pub(crate) fn new(content: &str) -> Generator {
        Generator {
            uri: None,
            version: None,
            content: content.trim().into(),
        }
    }
}

#[cfg(test)]
impl Generator {
    pub fn uri(mut self, uri: &str) -> Self {
        self.uri = Some(uri.to_owned());
        self
    }

    pub fn version(mut self, version: &str) -> Self {
        self.version = Some(version.to_owned());
        self
    }
}

/// Represents a a link to an image.
///
/// [Atom spec]:  http://www.atomenabled.org/developers/syndication/#optionalFeedElements
/// [RSS 2 spec]: https://validator.w3.org/feed/docs/rss2.html#ltimagegtSubelementOfLtchannelgt
/// [RSS 1 spec]: https://validator.w3.org/feed/docs/rss1.html#s5.4
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Image {
    /// Link to the image
    /// * Atom: The URL to an image or logo
    /// * RSS 1 + 2: the URL of a GIF, JPEG or PNG image that represents the channel.
    pub uri: String,
    /// RSS 1 + 2: describes the image, it's used in the ALT attribute of the HTML <img> tag when the channel is rendered in HTML.
    pub title: Option<String>,
    /// RSS 1 + 2: the URL of the site, when the channel is rendered, the image is a link to the site.
    pub link: Option<Link>,

    /// RSS 2 (optional): width of the image
    pub width: Option<u32>,
    /// RSS 2 (optional): height of the image
    pub height: Option<u32>,
    /// RSS 2 (optional): contains text that is included in the TITLE attribute of the link formed around the image in the HTML rendering.
    pub description: Option<String>,
}

impl Image {
    pub(crate) fn new(uri: String) -> Image {
        Image {
            uri,
            title: None,
            link: None,
            width: None,
            height: None,
            description: None,
        }
    }
}

#[cfg(test)]
impl Image {
    pub fn description(mut self, description: &str) -> Self {
        self.description = Some(description.to_owned());
        self
    }

    pub fn height(mut self, height: u32) -> Self {
        self.height = Some(height);
        self
    }

    pub fn link(mut self, link: &str) -> Self {
        self.link = Some(Link::new(link, None));
        self
    }

    pub fn title(mut self, title: &str) -> Self {
        self.title = Some(title.to_owned());
        self
    }

    pub fn width(mut self, width: u32) -> Self {
        self.width = Some(width);
        self
    }
}

/// Represents a link to an associated resource for the feed or entry.
///
/// [Atom spec]: http://www.atomenabled.org/developers/syndication/#link
#[derive(Clone, Debug, Eq, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Link {
    /// Link to additional content
    /// * Atom: The URI of the referenced resource (typically a Web page).
    /// * RSS 2: The URL to the HTML website corresponding to the channel or item.
    /// * JSON Feed: the URI to the attachment, feed etc
    pub href: String,
    /// A single link relationship type.
    pub rel: Option<String>,
    /// Indicates the media type of the resource.
    pub media_type: Option<String>,
    /// Indicates the language of the referenced resource.
    pub href_lang: Option<String>,
    /// Human readable information about the link, typically for display purposes.
    pub title: Option<String>,
    /// The length of the resource, in bytes.
    pub length: Option<u64>,
}

impl Link {
    pub(crate) fn new<S: AsRef<str>>(href: S, base: Option<&Url>) -> Link {
        let href = match util::parse_uri(href.as_ref(), base) {
            Some(uri) => uri.to_string(),
            None => href.as_ref().to_string(),
        }
        .trim()
        .to_string();

        Link {
            href,
            rel: None,
            media_type: None,
            href_lang: None,
            title: None,
            length: None,
        }
    }
}

#[cfg(test)]
impl Link {
    pub fn href_lang(mut self, lang: &str) -> Self {
        self.href_lang = Some(lang.to_owned());
        self
    }

    pub fn length(mut self, length: u64) -> Self {
        self.length = Some(length);
        self
    }

    pub fn media_type(mut self, media_type: &str) -> Self {
        self.media_type = Some(media_type.to_owned());
        self
    }

    pub fn rel(mut self, rel: &str) -> Self {
        self.rel = Some(rel.to_owned());
        self
    }

    pub fn title(mut self, title: &str) -> Self {
        self.title = Some(title.to_owned());
        self
    }
}

/// The top-level representation of a media object
/// i.e. combines "media:*" elements from the RSS Media spec such as those under a media:group
#[derive(Clone, Debug, Default, PartialEq)]
pub struct MediaObject {
    /// Title of the object (from the media:title element)
    pub title: Option<Text>,
    /// Collection of the media content elements
    pub content: Vec<MediaContent>,
    /// Duration of the object
    pub duration: Option<Duration>,
    /// Representative images for the object (from media:thumbnail elements)
    pub thumbnails: Vec<MediaThumbnail>,
    /// A text transcript, closed captioning or lyrics of the media content.
    pub texts: Vec<MediaText>,
    /// Short description of the media object (from the media:description element)
    pub description: Option<Text>,
    /// Community info (from the media:community element)
    pub community: Option<MediaCommunity>,
    /// Credits
    pub credits: Vec<MediaCredit>,
}

impl MediaObject {
    // Checks if this object has been populated with content
    pub(crate) fn has_content(&self) -> bool {
        self.title.is_some() || self.description.is_some() || !self.content.is_empty() || !self.thumbnails.is_empty() || !self.texts.is_empty()
    }
}

#[cfg(test)]
impl MediaObject {
    pub fn community(mut self, community: MediaCommunity) -> Self {
        self.community = Some(community);
        self
    }

    pub fn content(mut self, content: MediaContent) -> Self {
        self.content.push(content);
        self
    }

    pub fn credit(mut self, entity: &str) -> Self {
        self.credits.push(MediaCredit::new(entity.to_string()));
        self
    }

    pub fn description(mut self, description: &str) -> Self {
        self.description = Some(Text::new(description.to_string()));
        self
    }

    pub fn duration(mut self, duration: Duration) -> Self {
        self.duration = Some(duration);
        self
    }

    pub fn text(mut self, text: MediaText) -> Self {
        self.texts.push(text);
        self
    }

    pub fn thumbnail(mut self, thumbnail: MediaThumbnail) -> Self {
        self.thumbnails.push(thumbnail);
        self
    }

    pub fn title(mut self, title: &str) -> Self {
        self.title = Some(Text::new(title.to_string()));
        self
    }
}

/// Represents a "media:community" item from the RSS Media spec
#[derive(Clone, Debug, PartialEq)]
pub struct MediaCommunity {
    /// Star rating
    pub stars_avg: Option<f64>,
    pub stars_count: Option<u64>,
    pub stars_min: Option<u64>,
    pub stars_max: Option<u64>,

    /// Statistics on engagement
    pub stats_views: Option<u64>,
    pub stats_favorites: Option<u64>,
}

impl MediaCommunity {
    pub(crate) fn new() -> MediaCommunity {
        MediaCommunity {
            stars_avg: None,
            stars_count: None,
            stars_min: None,
            stars_max: None,
            stats_views: None,
            stats_favorites: None,
        }
    }
}

#[cfg(test)]
impl MediaCommunity {
    pub fn star_rating(mut self, count: u64, average: f64, min: u64, max: u64) -> Self {
        self.stars_count = Some(count);
        self.stars_avg = Some(average);
        self.stars_min = Some(min);
        self.stars_max = Some(max);
        self
    }

    pub fn statistics(mut self, views: u64, favorites: u64) -> Self {
        self.stats_views = Some(views);
        self.stats_favorites = Some(favorites);
        self
    }
}

/// Represents a "media:content" item from the RSS Media spec
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct MediaContent {
    /// The direct URL
    pub url: Option<Url>,
    /// Standard MIME type
    pub content_type: Option<Mime>,
    /// Height and width
    pub height: Option<u32>,
    pub width: Option<u32>,
    /// Duration the media plays
    pub duration: Option<Duration>,
    /// Size of media in bytes
    pub size: Option<u64>,
    /// Rating
    pub rating: Option<MediaRating>,
}

#[cfg(test)]
impl MediaContent {
    pub fn content_type(mut self, content_type: &str) -> Self {
        self.content_type = Some(content_type.parse::<Mime>().unwrap());
        self
    }

    pub fn height(mut self, height: u32) -> Self {
        self.height = Some(height);
        self
    }

    pub fn url(mut self, url: &str) -> Self {
        self.url = Some(Url::parse(url).unwrap());
        self
    }

    pub fn width(mut self, width: u32) -> Self {
        self.width = Some(width);
        self
    }

    pub fn duration(mut self, duration: Duration) -> Self {
        self.duration = Some(duration);
        self
    }

    pub fn size(mut self, size: u64) -> Self {
        self.size = Some(size);
        self
    }
}

impl MediaContent {
    pub(crate) fn new() -> MediaContent {
        MediaContent {
            url: None,
            content_type: None,
            height: None,
            width: None,
            duration: None,
            size: None,
            rating: None,
        }
    }
}

/// Represents a "media:credit" item from the RSS Media spec
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct MediaCredit {
    /// The entity being credited
    pub entity: String,
}

impl MediaCredit {
    pub(crate) fn new(entity: String) -> MediaCredit {
        MediaCredit { entity }
    }
}

/// Rating of the feed, item or media within the content
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct MediaRating {
    // The scheme (defaults to "simple" per the spec)
    pub urn: String,
    // The rating text
    pub value: String,
}

impl MediaRating {
    pub(crate) fn new(value: String) -> MediaRating {
        MediaRating { urn: "simple".into(), value }
    }

    pub fn urn(mut self, urn: &str) -> Self {
        self.urn = urn.to_string();
        self
    }
}

/// Represents a "media:text" item from the RSS Media spec
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct MediaText {
    /// The text
    pub text: Text,
    /// The start time offset that the text starts being relevant to the media object.
    pub start_time: Option<Duration>,
    /// The end time that the text is relevant. If this attribute is not provided, and a start time is used, it is expected that the end time is either the end of the clip or the start of the next <media:text> element.
    pub end_time: Option<Duration>,
}

impl MediaText {
    pub(crate) fn new(text: Text) -> MediaText {
        MediaText {
            text,
            start_time: None,
            end_time: None,
        }
    }
}

/// Represents a "media:thumbnail" item from the RSS Media spec
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct MediaThumbnail {
    /// The thumbnail image
    pub image: Image,
    /// The time this thumbnail represents
    pub time: Option<Duration>,
}

impl MediaThumbnail {
    pub(crate) fn new(image: Image) -> MediaThumbnail {
        MediaThumbnail { image, time: None }
    }
}

/// Represents an author, contributor etc.
///
/// [Atom spec]: http://www.atomenabled.org/developers/syndication/#person
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Person {
    /// Atom: human-readable name for the person.
    /// JSON Feed: human-readable name for the person.
    pub name: String,
    /// Atom: home page for the person.
    /// JSON Feed: link to media (Twitter etc) for the person
    pub uri: Option<String>,
    /// Atom: An email address for the person.
    pub email: Option<String>,
}

impl Person {
    pub(crate) fn new(name: &str) -> Person {
        Person {
            name: name.trim().into(),
            uri: None,
            email: None,
        }
    }

    pub fn email(mut self, email: &str) -> Self {
        self.email = Some(email.to_owned());
        self
    }
}

#[cfg(test)]
impl Person {
    pub fn uri(mut self, uri: &str) -> Self {
        self.uri = Some(uri.to_owned());
        self
    }
}

/// Textual content, or link to the content, for a given entry.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Text {
    pub content_type: Mime,
    pub src: Option<String>,
    pub content: String,
}

impl Text {
    pub(crate) fn new(content: String) -> Text {
        Text {
            content_type: mime::TEXT_PLAIN,
            src: None,
            content: content.trim().to_string(),
        }
    }

    pub(crate) fn html(content: String) -> Text {
        Text {
            content_type: mime::TEXT_HTML,
            src: None,
            content: content.trim().to_string(),
        }
    }
}

#[cfg(test)]
impl Text {
    pub fn content_type(mut self, content_type: &str) -> Self {
        self.content_type = content_type.parse::<Mime>().unwrap();
        self
    }
}
