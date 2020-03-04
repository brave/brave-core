extern crate readability;
extern crate url;

use readability::extractor::extract;
use std::fs::File;
use std::io::Read;
use url::Url;

static SAMPLES_PATH: &str = "./tests/samples/";

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

                let expected_string = load_test_files(stringify!($name));
                let product = extract(&mut source_f, &url).unwrap();

                assert_eq!(expected_string, product.content);
            }
        };
    }
}

//test!(aclu);
//test!(ars_1);
//test!(base_url);
//test!(base_url_base_element);
//test!(base_url_base_element_relative);
//test!(basic_tags_cleaning);
//test!(bbc_1);
//test!(blogger);
//test!(breitbart);
//test!(bug_1255978);
//test!(buzzfeed_1);
//test!(citylab_1);
//test!(clean_links);
//test!(cnet);
//test!(cnet_svg_classes);
//test!(cnn);
//test!(comment_inside_script_parsing);
//test!(daringfireball_1);
//test!(ehow_1);
//test!(ehow_2);
//test!(embedded_videos);
//test!(engadget);
//test!(folha);
//test!(gmw);
test!(guardian_1);
//test!(heise);
//test!(herald_sun_1);
//test!(hidden_nodes);
//test!(hukumusume);
//test!(iab_1);
//test!(ietf_1);
//test!(keep_images);
//test!(keep_tabular_data);
//test!(la_nacion);
//test!(lemonde_1);
//test!(liberation_1);
//test!(lifehacker_post_comment_load);
//test!(lifehacker_working);
//test!(links_in_tables);
//test!(lwn_1);
//test!(medicalnewstoday);
//test!(medium_1);
//test!(medium_3);
//test!(mercurial);
//test!(metadata_content_missing);
//test!(missing_paragraphs);
//test!(mozilla_1);
//test!(mozilla_2);
//test!(msn);
//test!(normalize_spaces);
//test!(nytimes_1);
//test!(nytimes_2);
//test!(nytimes_3);
//test!(nytimes_4);
//test!(pixnet);
//test!(qq);
//test!(remove_extra_brs);
//test!(remove_extra_paragraphs);
//test!(remove_script_tags);
//test!(reordering_paragraphs);
//test!(replace_brs);
//test!(replace_font_tags);
//test!(rtl_1);
//test!(rtl_2);
//test!(rtl_3);
//test!(rtl_4);
//test!(salon_1);
//test!(seattletimes_1);
//test!(simplyfound_3);
//test!(social_buttons);
//test!(style_tags_removal);
//test!(svg_parsing);
//test!(table_style_attributes);
//test!(telegraph);
//test!(title_and_h1_discrepancy);
//test!(tmz_1);
//test!(tumblr);
//test!(videos_1);
//test!(videos_2);
//test!(wapo_1);
//test!(wapo_2);
//test!(webmd_1);
//test!(webmd_2);
//test!(wikipedia);
//test!(wordpress);
//test!(yahoo_1);
//test!(yahoo_2);
//test!(yahoo_3);
//test!(yahoo_4);
//test!(youth);
