use lol_html::html_content::*;
use lol_html::ElementContentHandlers;
use lol_html::Selector;
use std::error::Error;

use crate::speedreader::RewriteRules;
use crate::speedreader::SpeedReaderError;

pub type HandlerResult = Result<(), Box<dyn Error>>;
pub type ElementHandler = Box<dyn Fn(&mut Element) -> HandlerResult>;
pub type TextHandler = Box<dyn Fn(&mut TextChunk) -> HandlerResult>;

pub struct ContentFunction {
    pub element: Option<ElementHandler>,
    pub text: Option<TextHandler>,
}

impl From<ElementHandler> for ContentFunction {
    #[inline]
    fn from(handler: ElementHandler) -> Self {
        ContentFunction {
            element: Some(handler),
            text: None,
        }
    }
}

impl From<TextHandler> for ContentFunction {
    #[inline]
    fn from(handler: TextHandler) -> Self {
        ContentFunction {
            element: None,
            text: Some(handler),
        }
    }
}

pub fn rewrite_rules_to_content_handlers(
    conf: &RewriteRules,
    origin: &str,
) -> Vec<(Selector, ContentFunction)> {
    let mut element_content_handlers = vec![];
    let mut errors = vec![];

    for attr_rewrite in &conf.preprocess {
        let rewrite = attr_rewrite.clone();
        add_element_function(
            &mut element_content_handlers,
            &mut errors,
            &attr_rewrite.selector,
            Box::new(move |el| {
                if let Some((attribute_from, attribute_to)) = rewrite.attribute.as_ref() {
                    if let Some(attr_value) = el.get_attribute(attribute_from) {
                        el.set_attribute(&attribute_to, &attr_value).unwrap_or(());
                    }
                }
                el.set_tag_name(&rewrite.element_name)?;
                Ok(())
            }),
        );
    }

    // Keep the basic HTML structure
    add_element_function(
        &mut element_content_handlers,
        &mut errors,
        "html, html > head, html > body",
        Box::new(mark_retained_element),
    );

    add_element_function(
        &mut element_content_handlers,
        &mut errors,
        "head",
        Box::new(|el: &mut Element| {
            el.prepend(
                r#"<link rel="stylesheet" href="/readermode.css" type="text/css">"#,
                ContentType::Html,
            );
            Ok(())
        }),
    );

    let maybe_script = conf.content_script.clone();
    add_element_function(
        &mut element_content_handlers,
        &mut errors,
        "body",
        Box::new(move |el| {
            el.before("<article id=\"article\">", ContentType::Html);
            el.after("</article>", ContentType::Html);
            if let Some(script) = &maybe_script {
                el.append(&script, ContentType::Html);
            }
            el.set_attribute("dir", "auto")?;
            Ok(())
        }),
    );

    collect_main_content(
        &mut element_content_handlers,
        &mut errors,
        &conf.get_main_content_selectors(),
        &conf.get_content_cleanup_selectors(),
    );
    if conf.delazify {
        delazify(&mut element_content_handlers, &mut errors);
    }
    if conf.fix_embeds {
        fix_social_embeds(&mut element_content_handlers, &mut errors);
    }
    correct_relative_links(
        &mut element_content_handlers,
        &mut errors,
        origin.to_owned(),
    );

    if !errors.is_empty() {
        eprintln!(
            "Rewriter rules include invalid content selectors: {}",
            errors
                .iter()
                .map(|e| e.to_string())
                .collect::<Vec<_>>()
                .join(", ")
        )
    }

    element_content_handlers
}

pub fn content_handlers<'h>(
    handlers: &'h [(Selector, ContentFunction)],
) -> Vec<(&Selector, ElementContentHandlers<'h>)> {
    handlers
        .iter()
        .map(|(selector, function)| (selector, get_content_handlers(function)))
        .collect::<Vec<_>>()
}

#[inline]
fn get_content_handlers(function: &ContentFunction) -> ElementContentHandlers<'_> {
    if let Some(f_element) = function.element.as_ref() {
        ElementContentHandlers::default().element(f_element)
    } else if let Some(t_element) = function.text.as_ref() {
        ElementContentHandlers::default().text(t_element)
    } else {
        ElementContentHandlers::default()
    }
}

#[inline]
fn add_element_function(
    handlers: &mut Vec<(Selector, ContentFunction)>,
    errors: &mut Vec<SpeedReaderError>,
    selector: &str,
    handler: ElementHandler,
) {
    match selector.parse::<Selector>() {
        Ok(selector) => handlers.push((selector, ContentFunction::from(handler))),
        Err(error) => errors.push(SpeedReaderError::SelectorError(selector.to_owned(), error)),
    }
}

#[inline]
fn add_text_function(
    handlers: &mut Vec<(Selector, ContentFunction)>,
    errors: &mut Vec<SpeedReaderError>,
    selector: &str,
    handler: TextHandler,
) {
    match selector.parse::<Selector>() {
        Ok(selector) => handlers.push((selector, ContentFunction::from(handler))),
        Err(error) => errors.push(SpeedReaderError::SelectorError(selector.to_owned(), error)),
    }
}

#[inline]
fn collect_main_content(
    handlers: &mut Vec<(Selector, ContentFunction)>,
    errors: &mut Vec<SpeedReaderError>,
    content_selectors: &[&str],
    cleanup_selectors: &[&str],
) {
    content_selectors.iter().for_each(|selector| {
        add_element_function(
            handlers,
            errors,
            &format!("{}, {} *", selector, selector),
            Box::new(mark_retained_element),
        );
        add_text_function(
            handlers,
            errors,
            &format!("{}, {} *", selector, selector),
            Box::new(mark_retained_text),
        );
    });

    cleanup_selectors.iter().for_each(|selector| {
        add_element_function(
            handlers,
            errors,
            selector,
            Box::new(|el| {
                el.remove();
                Ok(())
            }),
        );
    });

    // Drop everything else
    add_text_function(handlers, errors, "*", Box::new(remove_unmarked_text));
    add_element_function(handlers, errors, "*", Box::new(unwrap_unmarked_element));
    add_element_function(
        handlers,
        errors,
        "[style]",
        Box::new(|el| {
            el.remove_attribute("style");
            Ok(())
        }),
    );
}

#[inline]
fn correct_relative_links(
    handlers: &mut Vec<(Selector, ContentFunction)>,
    errors: &mut Vec<SpeedReaderError>,
    origin: String,
) {
    let href_origin = origin.clone();
    add_element_function(
        handlers,
        errors,
        "a[href]",
        Box::new(move |el| {
            let href = el.get_attribute("href").expect("href was required");

            if !href.starts_with("http://")
                && !href.starts_with("https://")
                && !href.starts_with("//")
            {
                el.set_attribute("href", &format!("{}{}", href_origin, href))?;
            }

            Ok(())
        }),
    );
    add_element_function(
        handlers,
        errors,
        "img[src]",
        Box::new(move |el| {
            let src = el.get_attribute("src").expect("src was required");

            if !src.starts_with("http://") && !src.starts_with("https://") && !src.starts_with("//")
            {
                el.set_attribute("src", &format!("{}{}", origin, src))?;
            }

            Ok(())
        }),
    );
}

const LAZY_MAPPINGS: [(&str, &str, &str); 6] = [
    ("[data-src]", "data-src", "src"),
    ("[data-srcset]", "data-srcset", "srcset"),
    ("[data-original]", "data-original", "src"),
    ("img[data-src-medium]", "data-src-medium", "src"),
    ("img[data-raw-src]", "data-raw-src", "src"),
    ("img[data-gl-src]", "data-gl-src", "src"),
];

#[inline]
fn delazify(handlers: &mut Vec<(Selector, ContentFunction)>, errors: &mut Vec<SpeedReaderError>) {
    for (selector, attribute, target) in LAZY_MAPPINGS.iter() {
        add_element_function(
            handlers,
            errors,
            selector,
            Box::new(move |el| {
                if let Some(src) = el.get_attribute(attribute) {
                    el.set_attribute(target, &src).ok();
                }
                Ok(())
            }),
        )
    }

    add_element_function(
        handlers,
        errors,
        "img",
        Box::new(|el| {
            el.remove_attribute("height");
            el.remove_attribute("width");
            Ok(())
        }),
    );
}

#[inline]
fn fix_social_embeds(
    handlers: &mut Vec<(Selector, ContentFunction)>,
    errors: &mut Vec<SpeedReaderError>,
) {
    add_element_function(
        handlers,
        errors,
        ".twitterContainer",
        Box::new(|el: &mut Element| {
            el.prepend(
                r#"
            <script type="text/javascript" src="//platform.twitter.com/widgets.js" async="">
            </script>"#,
                ContentType::Html,
            );
            Ok(())
        }),
    );
}

#[inline]
fn mark_retained_element(el: &mut Element) -> HandlerResult {
    el.set_user_data(true);
    Ok(())
}

#[inline]
fn mark_retained_text(t: &mut TextChunk) -> HandlerResult {
    t.set_user_data(true);
    Ok(())
}

#[inline]
fn remove_unmarked_text(t: &mut TextChunk) -> HandlerResult {
    let user_data = t.user_data_mut().downcast_ref::<bool>();
    if user_data != Some(&true) {
        t.remove();
        Ok(())
    } else {
        Ok(())
    }
}

#[inline]
fn unwrap_unmarked_element(el: &mut Element) -> HandlerResult {
    let user_data = el.user_data_mut().downcast_ref::<bool>();
    if user_data != Some(&true) {
        el.remove_and_keep_content();
        Ok(())
    } else {
        Ok(())
    }
}
