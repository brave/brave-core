#![allow(dead_code)]
extern crate distance;
extern crate html5ever;
extern crate readability;
extern crate regex;
extern crate speedreader;
extern crate url;

#[macro_use]
extern crate markup5ever;

#[macro_use]
extern crate lazy_static;

use readability::extractor;
use speedreader::classifier::feature_extractor::FeatureExtractorStreamer;
use std::collections::HashSet;
use std::fs::File;
use std::io::Read;
use url::Url;

use markup5ever_rcdom::NodeData::{Element, Text};
use markup5ever_rcdom::RcDom;
use markup5ever_rcdom::{Handle, Node};
use regex::Regex;
use std::rc::Rc;
use std::vec::Vec;

static SAMPLES_PATH: &str = "./tests/samples/";

fn load_test_files(test_name: &str) -> String {
    let mut expected = String::new();
    let mut exp_f = File::open(format!("{}/{}/expected.html", SAMPLES_PATH, test_name)).unwrap();
    exp_f.read_to_string(&mut expected).unwrap();
    expected
}

pub fn extract_flattened_tree<S: ::std::hash::BuildHasher>(
    handle: Handle,
    tags_extracted: &HashSet<String, S>,
    flattened_nodes: &mut Vec<Rc<Node>>,
) -> Vec<Rc<Node>> {
    for child in handle.children.borrow().iter() {
        let c = child.clone();
        match c.data {
            Text { .. } => {
                flattened_nodes.push(c.clone());
            }
            Element { ref name, .. } => {
                let tag = name.local.as_ref();
                let tag_name = tag.to_lowercase();

                if tags_extracted.contains(&tag_name) {
                    flattened_nodes.push(c.clone());
                }

                // if type Element, traverse to children in next iteration
                extract_flattened_tree(child.clone(), tags_extracted, flattened_nodes);
            }
            _ => (),
        }
    }
    flattened_nodes.to_vec()
}

pub fn extract_text(handle: &Handle) -> String {
    let node_text = match handle.data {
        Text { ref contents } => Some(contents.borrow().trim().to_string()),
        Element {
            ref name,
            ref attrs,
            ..
        } if name.local == local_name!("img") => {
            let attrs_borrow = attrs.borrow();
            let attr = attrs_borrow
                .iter()
                .find(|attr| attr.name.local == local_name!("src"));
            let attr_value: Option<String> = attr.map(|a| a.value.to_string());
            Some(format!("<img src='{:?}'/>", attr_value))
        }
        Element {
            ref name,
            ref attrs,
            ..
        } if name.local == local_name!("a") => {
            let attrs_borrow = attrs.borrow();
            let attr = attrs_borrow
                .iter()
                .find(|attr| attr.name.local == local_name!("href"));
            let attr_value: Option<String> = attr.map(|a| a.value.to_string());
            Some(format!("<a href='{:?}'/>", attr_value))
        }
        _ => None,
    };

    let contents = if let Some(text) = node_text {
        vec![text]
    } else {
        vec![]
    };
    contents.join(" ")
}

fn lcs(left: &[String], right: &[String]) -> (usize, Vec<String>) {
    let total_rows = left.len() + 1;
    let total_columns = right.len() + 1;
    let mut table = vec![vec![0; total_columns]; total_rows];
    for (row, left_text) in left.iter().enumerate() {
        for (col, right_text) in right.iter().enumerate() {
            if left_text == right_text {
                table[row + 1][col + 1] = table[row][col] + 1;
            } else {
                table[row + 1][col + 1] = std::cmp::max(table[row + 1][col], table[row][col + 1]);
            }
        }
    }
    let mut common_seq = Vec::new();
    let mut x = total_rows - 1;
    let mut y = total_columns - 1;
    while x != 0 && y != 0 {
        // Check element above is equal
        if table[x][y] == table[x - 1][y] {
            x -= 1;
        }
        // check element to the left is equal
        else if table[x][y] == table[x][y - 1] {
            y -= 1;
        } else {
            // check the two element at the respective x,y position is same
            assert_eq!(left[x - 1], right[y - 1]);
            let text = left[x - 1].to_owned();
            common_seq.push(text);
            x -= 1;
            y -= 1;
        }
    }
    common_seq.reverse();
    (table[total_rows - 1][total_columns - 1], common_seq)
}

fn get_flat_dom_nodes(dom: &RcDom) -> Vec<String> {
    let mut expected_nodes = Vec::new();
    // checks full flattened tree for a subset of (tags, attrs)
    let mut tags = HashSet::new();
    // #TODO: check a tags and imgs too, but for now focus on text
    tags.insert("a".to_owned());
    //tags.insert("img".to_owned());
    extract_flattened_tree(dom.document.clone(), &tags, &mut expected_nodes);

    lazy_static! {
        static ref WHITESPACE: Regex = Regex::new(r"(\s\s+)").unwrap();
        static ref NEWLINE_ESCAPED: Regex = Regex::new(r"(\\n)").unwrap();
    }

    expected_nodes
        .iter()
        .map(|n| extract_text(n))
        .map(|t| {
            let repl = NEWLINE_ESCAPED.replace_all(&t, " ");
            let repl = WHITESPACE.replace_all(&repl, " ");
            format!("{}", repl)
        })
        .filter(|t| !t.is_empty())
        .collect()
}

fn generate_comparison(left: &[String], right: &[String], lcs: &[String]) -> String {
    let mut left_iter = left.iter();
    let mut right_iter = right.iter();
    let mut output = "\n".to_owned();
    for common in lcs {
        while let Some(left) = left_iter.next() {
            if left == common {
                break;
            } else {
                output.push_str(&format!(
                    "{} {} {}\n",
                    termion::color::Fg(termion::color::Red),
                    left,
                    termion::color::Fg(termion::color::Reset)
                ));
            }
        }

        while let Some(right) = right_iter.next() {
            if right == common {
                break;
            } else {
                output.push_str(&format!(
                    "{} {} {}\n",
                    termion::color::Fg(termion::color::Yellow),
                    right,
                    termion::color::Fg(termion::color::Reset)
                ));
            }
        }

        output.push_str(&format!(
            "{} {} {}\n",
            termion::color::Fg(termion::color::Reset),
            common,
            termion::color::Fg(termion::color::Reset)
        ));
    }

    for left in left_iter {
        output.push_str(&format!(
            "{} {} {}\n",
            termion::color::Fg(termion::color::Red),
            left,
            termion::color::Fg(termion::color::Reset)
        ));
    }

    for right in right_iter {
        output.push_str(&format!(
            "{} {} {}\n",
            termion::color::Fg(termion::color::Yellow),
            right,
            termion::color::Fg(termion::color::Reset)
        ));
    }
    output
}

fn test_contents(name: &str) {
    let url = Url::parse("http://url.com").unwrap();
    let mut source_f = File::open(format!("{}/{}/source.html", SAMPLES_PATH, name)).unwrap();

    // opens and parses the expected final result into a rcdom
    // (for comparing with the result)
    let expected_string = load_test_files(stringify!($name));
    let mut feature_extractor = FeatureExtractorStreamer::try_new(&url).unwrap();
    feature_extractor
        .write(&mut expected_string.as_bytes())
        .unwrap();
    let expected = feature_extractor.end();

    let expected_nodes_str = get_flat_dom_nodes(&expected.rcdom);

    // uses the mapper build the mapper based on the source HTML
    // document
    let product = extractor::extract(&mut source_f, &url).unwrap();
    let mut feature_extractor = FeatureExtractorStreamer::try_new(&url).unwrap();
    feature_extractor
        .write(&mut product.content.as_bytes())
        .unwrap();
    let result = feature_extractor.end();

    let got_nodes_str = get_flat_dom_nodes(&result.rcdom);

    let (_, subsequence) = lcs(&expected_nodes_str, &got_nodes_str);

    assert!(
        expected_nodes_str == got_nodes_str,
        "Not equal: {}",
        generate_comparison(&expected_nodes_str, &got_nodes_str, &subsequence)
    );
}

// macro_rules! test_str {
//     ($name:ident) => {
//         #[test]
//         fn $name() {
//             test_contents(stringify!($name))
//         }
//     }
// }

// // - salon_1 has whole front of an article missing
// test_str!(salon_1);

// // - wapo_2 doesn’t include key article images
// test_str!(wapo_2);

// // - telegraph misses half an article
// test_str!(telegraph);

// // - medium_3 misses multiple apragraphs at front _and_ end
// test_str!(medium_3);

// // - nytimes_2 misses entire front including image and paragraphs
// test_str!(nytimes_2);

// // - nytimes_4, nytimes_3 misses most of the article
// test_str!(nytimes_4);

// // - mozilla_1 misses most of article
// test_str!(mozilla_1);

// // - ehow_2 misses most of article
// test_str!(ehow_2);

// test_str!(ars_1);
// test_str!(cnet);
// test_str!(folha);
// test_str!(liberation_1);
// test_str!(metadata_content_missing);
// test_str!(msn);
// test_str!(rtl_1);
// test_str!(rtl_2);
// test_str!(rtl_3);
// test_str!(rtl_4);
// test_str!(tumblr);
// test_str!(yahoo_4);
// test_str!(videos_2);
// test_str!(pixnet);
// test_str!(aclu);
// test_str!(base_url);
// test_str!(base_url_base_element);
// test_str!(base_url_base_element_relative);
// test_str!(basic_tags_cleaning);
// test_str!(guardian_1);
// test_str!(heise);
// test_str!(embedded_videos);
// test_str!(lemonde_1);
// test_str!(lifehacker_post_comment_load);
// test_str!(lifehacker_working);
// test_str!(youth);
// test_str!(social_buttons);
// test_str!(style_tags_removal);
// test_str!(normalize_spaces);
// test_str!(nytimes_1);
// test_str!(missing_paragraphs);
// test_str!(replace_font_tags);
// test_str!(reordering_paragraphs);
// test_str!(videos_1);
// test_str!(breitbart);

// test_str!(bbc_1);

// test_str!(wordpress);
// test_str!(bug_1255978);
// test_str!(buzzfeed_1);
// test_str!(citylab_1);
// test_str!(clean_links);
// test_str!(cnet_svg_classes);
// test_str!(cnn);
// test_str!(comment_inside_script_parsing);
// test_str!(daringfireball_1);
// test_str!(ehow_1);
// test_str!(engadget);
// test_str!(gmw);
// test_str!(herald_sun_1);
// test_str!(hidden_nodes);
// test_str!(hukumusume);
// test_str!(iab_1);
// test_str!(ietf_1);
// test_str!(keep_images);
// test_str!(keep_tabular_data);
// test_str!(la_nacion);
// test_str!(links_in_tables);
// test_str!(lwn_1);
// test_str!(medicalnewstoday);
// test_str!(medium_1);
// test_str!(mercurial);
// test_str!(mozilla_2);
// test_str!(nytimes_3);
// test_str!(qq);
// test_str!(remove_extra_brs);
// test_str!(remove_extra_paragraphs);
// test_str!(remove_script_tags);
// test_str!(replace_brs);
// test_str!(seattletimes_1);
// test_str!(simplyfound_3);
// test_str!(svg_parsing);
// test_str!(table_style_attributes);
// test_str!(tmz_1);
// test_str!(wapo_1);
// test_str!(webmd_1);
// test_str!(webmd_2);
// test_str!(wikipedia);
// test_str!(yahoo_1);
// test_str!(yahoo_2);
// test_str!(yahoo_3);
