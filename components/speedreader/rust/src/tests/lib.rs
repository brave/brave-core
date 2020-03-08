#![allow(dead_code)]
extern crate distance;
extern crate html5ever;
extern crate readability;
extern crate speedreader;
extern crate url;

use readability::extractor::extract;
use speedreader::classifier::feature_extractor::FeatureExtractorStreamer;
use std::fs::File;
use std::io::Read;
use url::Url;

use distance::damerau_levenshtein;
use markup5ever_rcdom::NodeData::{Element, Text};
use markup5ever_rcdom::{Handle, Node};
use std::rc::Rc;
use std::vec::Vec;

static SAMPLES_PATH: &str = "./tests/samples/";

pub fn extract_flattened_tree(
    handle: Handle,
    tags_attrs: Vec<(&str, &str)>,
    flattened_nodes: &mut Vec<Rc<Node>>,
) -> Vec<Rc<Node>> {
    for child in handle.children.borrow().iter() {
        let c = child.clone();
        match c.data {
            Text { .. } => {
                flattened_nodes.push(c.clone());
            }
            Element {
                ref name,
                ref attrs,
                ..
            } => {
                let t = name.local.as_ref();
                for a in attrs.borrow().iter() {
                    let t = t.to_lowercase();
                    let a = a.value.to_string().to_lowercase();

                    // check if current node name and attr match expected
                    for ta in tags_attrs.clone() {
                        let (tag_name, attr_name): (&str, &str) = ta;
                        if t == tag_name && a == attr_name {
                            flattened_nodes.push(c.clone());
                        }
                    }
                }
                // if type Element, traverse to children in next iteration
                extract_flattened_tree(child.clone(), tags_attrs.clone(), flattened_nodes);
            }
            _ => (),
        }
    }
    flattened_nodes.to_vec()
}

// recursively extracts all text of leaf nodes into a string for comparison
pub fn extract_text(handle: Handle, text: &mut String) {
    for child in handle.children.borrow().iter() {
        let c = child.clone();
        match c.data {
            Text { ref contents } => {
                text.push_str(contents.borrow().trim());
            }
            Element { .. } => {
                extract_text(child.clone(), text);
            }
            _ => (),
        }
    }
}

// recursively collects values of nodes with a certain tuple (tag_id, attribute_id)
// into a vector of strings for comparison
fn stripped_content(
    handle: Handle,
    tag_name: &str,
    attr_name: &str,
    nodes: &mut Vec<Rc<Node>>,
    values: &mut Vec<String>,
) {
    for child in handle.children.borrow().iter() {
        if let Element {
            ref name,
            ref attrs,
            ..
        } = child.data
        {
            let t = name.local.as_ref();
            if t.to_lowercase() == tag_name {
                nodes.push(child.clone());

                for attr in attrs.borrow().iter() {
                    if attr.name.local.as_ref() == attr_name {
                        values.push(attr.value.to_string());
                    }
                }
            };
            stripped_content(child.clone(), tag_name, attr_name, nodes, values);
        }
    }
}

// compares if DOMs keep an approximate (to a factor) number and value of the tuple
// (tag_name, attr_name)
fn tags_match_approx(
    d1: Handle,
    d2: Handle,
    tag_name: &str,
    attr_name: &str,
    approx_factor: usize,
) -> bool {
    let mut values_d1 = Vec::new();
    let mut values_d2 = Vec::new();
    stripped_content(d1, tag_name, attr_name, &mut Vec::new(), &mut values_d1);
    stripped_content(d2, tag_name, attr_name, &mut Vec::new(), &mut values_d2);

    if values_d2.len() > values_d1.len() + approx_factor {
        println!("{:#?}\n != \n{:#?}", values_d1.len(), values_d2.len());
        return false;
    }

    values_d1.sort();
    values_d2.sort();

    let mut approx_counter = approx_factor;
    for (i, _) in values_d1.clone().iter().enumerate() {
        if values_d2.len() > i && values_d1[i] != values_d2[i] {
            approx_counter -= 1;
            if approx_counter == 0 {
                return false;
            }
        }
    }
    true
}

// stricly compares if flattened tree with subset of (tags, attrs) match
fn flattened_tree_match_strict(d1: Handle, d2: Handle, tags_attrs: Vec<(&str, &str)>) -> bool {
    let _ftree1 = extract_flattened_tree(d1, tags_attrs.clone(), &mut Vec::new());
    let _ftree2 = extract_flattened_tree(d2, tags_attrs, &mut Vec::new());

    // #TODO: compare nodes' content
    //for (i, _) in ftree1.clone().iter().enumerate() {
    //   if ftree1[i] != ftree2[i] {
    //        return false;
    //}

    true
}

fn strings_match_approx(s1: &str, s2: &str, f: usize) -> bool {
    let diff = damerau_levenshtein(s1, s2);
    if diff > f {
        println!("damerau_levenshtein:: {}", diff);
        return false;
    }
    true
}

fn load_test_files(test_name: &str) -> String {
    let mut expected = "".to_owned();
    let mut exp_f = File::open(format!("{}/{}/expected.html", SAMPLES_PATH, test_name)).unwrap();
    exp_f.read_to_string(&mut expected).unwrap();

    expected
}

#[macro_use]
#[cfg(test)]
mod test {
    macro_rules! test {
        ($name:ident) => {
            #[test]
            fn $name() {
                let url = Url::parse("http://url.com").unwrap();
                let mut source_f = File::open(format!(
                    "{}/{}/source.html",
                    SAMPLES_PATH,
                    stringify!($name)
                ))
                .unwrap();

                // opens and parses the expected final result into a rcdom
                // (for comparing with the result)
                let expected_string = load_test_files(stringify!($name));
                let mut feature_extractor = FeatureExtractorStreamer::try_new(&url).unwrap();
                feature_extractor
                    .write(&mut expected_string.as_bytes())
                    .unwrap();
                let expected = feature_extractor.end();

                // uses the mapper build the mapper based on the source HTML
                // document
                let product = extract(&mut source_f, &url).unwrap();
                let mut feature_extractor = FeatureExtractorStreamer::try_new(&url).unwrap();
                feature_extractor
                    .write(&mut product.content.as_bytes())
                    .unwrap();
                let result = feature_extractor.end();

                // checks full flattened tree for a subset of (tags, attrs)
                //let mut tags_attrs: Vec<(&str, &str)> = Vec::new();
                //tags_attrs.push(("a", "href"));
                //tags_attrs.push(("img", "src"));

                //let flattened_tree_match = flattened_tree_match_strict(
                //   expected.dom.document.clone(),
                //   result.dom.document.clone(),
                //   tags_attrs);

                //assert!(flattened_tree_match, "Full flattened trees do not strictly match");

                let atags_match = tags_match_approx(
                    expected.rcdom.document.clone(),
                    result.rcdom.document.clone(),
                    "a",
                    "href",
                    5,
                );

                assert!(
                    atags_match,
                    "Node values of <a href=''> do not approximately match"
                );

                let imgtags_match = tags_match_approx(
                    expected.rcdom.document.clone(),
                    result.rcdom.document.clone(),
                    "img",
                    "src",
                    5,
                );

                assert!(
                    imgtags_match,
                    "Node values of <img src=''> do not strictly match"
                );

                // note: now we can define tests similar to tags_match_strict
                // but that are less strict. e.g. number of nodes in dom of a
                // certain (tag, attr) may be differ by x)

                // compares full flattened text nodes
                let levenstein_threshold = 900;
                let mut text_result = String::new();
                extract_text(result.rcdom.document.clone(), &mut text_result);
                let mut text_expected = String::new();
                extract_text(expected.rcdom.document.clone(), &mut text_expected);

                let strings_approx =
                    strings_match_approx(&text_result, &text_expected, levenstein_threshold);
                assert!(strings_approx, "Flattened text is not similar enough");
                //assert_eq!(text_result, text_expected, "Falttened texts in p tags do not match");
            }
        };
    }
}

// passing

#[cfg(test)]
mod fulltest {
    use super::*;
    test!(ars_1);
    test!(cnet);
    test!(folha);
    test!(liberation_1);
}
// test!(metadata_content_missing);
// test!(msn);
// test!(rtl_1);
// test!(rtl_2);
// test!(rtl_3);
// test!(rtl_4);
// test!(title_and_h1_discrepancy);
// test!(tumblr);
// test!(yahoo_4);
// test!(videos_2);
// test!(wordpress);
// test!(pixnet);

// // not passing in strict mode

// test!(aclu);
// test!(base_url);
// test!(base_url_base_element);
// test!(base_url_base_element_relative);
// test!(basic_tags_cleaning);
// test!(bbc_1);
// test!(blogger);
// test!(breitbart);
// test!(bug_1255978);
// test!(buzzfeed_1);
// test!(citylab_1);
// test!(clean_links);
// test!(cnet_svg_classes);
// test!(cnn);
// test!(comment_inside_script_parsing);
// test!(daringfireball_1);
// test!(ehow_1);
// test!(ehow_2);
// test!(embedded_videos);
// test!(engadget);
// test!(gmw);
// test!(guardian_1);
// test!(heise);
// test!(herald_sun_1);
// test!(hidden_nodes);
// test!(hukumusume);
// test!(iab_1);
// test!(ietf_1);
// test!(keep_images);
// test!(keep_tabular_data);
// test!(la_nacion);
// test!(lemonde_1);
// test!(lifehacker_post_comment_load);
// test!(lifehacker_working);
// test!(links_in_tables);
// test!(lwn_1);
// test!(medicalnewstoday);
// test!(medium_1);
// test!(medium_3);
// test!(mercurial);
// test!(missing_paragraphs);
// test!(mozilla_1);
// test!(mozilla_2);
// test!(normalize_spaces);
// test!(nytimes_1);
// test!(nytimes_2);
// test!(nytimes_3);
// test!(nytimes_4);
// test!(qq);
// test!(remove_extra_brs);
// test!(remove_extra_paragraphs);
// test!(remove_script_tags);
// test!(reordering_paragraphs);
// test!(replace_brs);
// test!(replace_font_tags);
// test!(salon_1);
// test!(seattletimes_1);
// test!(simplyfound_3);
// test!(social_buttons);
// test!(style_tags_removal);
// test!(svg_parsing);
// test!(table_style_attributes);
// test!(telegraph);
// test!(tmz_1);
// test!(videos_1);
// test!(wapo_1);
// test!(wapo_2);
// test!(webmd_1);
// test!(webmd_2);
// test!(wikipedia);
// test!(yahoo_1);
// test!(yahoo_2);
// test!(yahoo_3);
// test!(youth);
