use std::collections::HashMap;

use crate::{
    file::SectionId,
    parse::tests::util::{OwnedEvent, OwnedHeader, own_event, own_header},
};

mod try_from {
    use std::collections::HashMap;

    use super::{bodies, headers};
    use crate::{
        File,
        file::{SectionId, SectionLookup},
        parse::{
            section,
            tests::util::{OwnedEvent as Event, name_event, newline_event, section_header, value_event},
        },
    };

    #[test]
    fn empty() {
        let config = File::try_from("").unwrap();
        assert_eq!(config.next_section_id, 0);
        assert!(config.section_lookup_tree.is_empty());
        assert!(config.sections.is_empty());
        assert!(config.section_order.is_empty());
    }

    #[test]
    fn single_section() {
        let config = File::try_from("[core]\na=b\nc=d").unwrap();
        let expected_separators = {
            let mut map = HashMap::new();
            map.insert(SectionId(0), section_header("core", None));
            map
        };
        assert_eq!(headers(&config), expected_separators);
        assert_eq!(config.next_section_id, 1);
        let expected_lookup_tree = {
            let mut tree = HashMap::new();
            tree.insert(
                section::Name("core".into()),
                SectionLookup {
                    without_subsection: vec![SectionId(0)],
                    ..Default::default()
                },
            );
            tree
        };
        assert_eq!(config.section_lookup_tree, expected_lookup_tree);
        let expected_sections = {
            let mut sections = HashMap::new();
            sections.insert(
                SectionId(0),
                vec![
                    newline_event(),
                    name_event("a"),
                    Event::KeyValueSeparator,
                    value_event("b"),
                    newline_event(),
                    name_event("c"),
                    Event::KeyValueSeparator,
                    value_event("d"),
                ],
            );
            sections
        };
        assert_eq!(bodies(&config), expected_sections);
        assert_eq!(config.section_order, [SectionId(0)]);
    }

    #[test]
    fn single_subsection() {
        let config = File::try_from("[core.sub]\na=b\nc=d").unwrap();
        let expected_separators = {
            let mut map = HashMap::new();
            map.insert(SectionId(0), section_header("core", (".", "sub")));
            map
        };
        assert_eq!(headers(&config), expected_separators);
        assert_eq!(config.next_section_id, 1);
        let expected_lookup_tree = {
            let mut tree = HashMap::new();
            let mut inner_tree = HashMap::new();
            inner_tree.insert("sub".into(), vec![SectionId(0)]);
            tree.insert(
                section::Name("core".into()),
                SectionLookup {
                    by_subsection: inner_tree,
                    ..Default::default()
                },
            );
            tree
        };
        assert_eq!(config.section_lookup_tree, expected_lookup_tree);
        let expected_sections = {
            let mut sections = HashMap::new();
            sections.insert(
                SectionId(0),
                vec![
                    newline_event(),
                    name_event("a"),
                    Event::KeyValueSeparator,
                    value_event("b"),
                    newline_event(),
                    name_event("c"),
                    Event::KeyValueSeparator,
                    value_event("d"),
                ],
            );
            sections
        };
        assert_eq!(bodies(&config), expected_sections);
        assert_eq!(config.section_order, [SectionId(0)]);
    }

    #[test]
    fn multiple_sections() {
        let config = File::try_from("[core]\na=b\nc=d\n[other]e=f").unwrap();
        let expected_separators = {
            let mut map = HashMap::new();
            map.insert(SectionId(0), section_header("core", None));
            map.insert(SectionId(1), section_header("other", None));
            map
        };
        assert_eq!(headers(&config), expected_separators);
        assert_eq!(config.next_section_id, 2);
        let expected_lookup_tree = {
            let mut tree = HashMap::new();
            tree.insert(
                section::Name("core".into()),
                SectionLookup {
                    without_subsection: vec![SectionId(0)],
                    ..Default::default()
                },
            );
            tree.insert(
                section::Name("other".into()),
                SectionLookup {
                    without_subsection: vec![SectionId(1)],
                    ..Default::default()
                },
            );
            tree
        };
        assert_eq!(config.section_lookup_tree, expected_lookup_tree);
        let expected_sections = {
            let mut sections = HashMap::new();
            sections.insert(
                SectionId(0),
                vec![
                    newline_event(),
                    name_event("a"),
                    Event::KeyValueSeparator,
                    value_event("b"),
                    newline_event(),
                    name_event("c"),
                    Event::KeyValueSeparator,
                    value_event("d"),
                    newline_event(),
                ],
            );
            sections.insert(
                SectionId(1),
                vec![name_event("e"), Event::KeyValueSeparator, value_event("f")],
            );
            sections
        };
        assert_eq!(bodies(&config), expected_sections);
        assert_eq!(config.section_order, [SectionId(0), SectionId(1)]);
    }

    #[test]
    fn multiple_duplicate_sections() {
        let config = File::try_from("[core]\na=b\nc=d\n[core]e=f").unwrap();
        let expected_separators = {
            let mut map = HashMap::new();
            map.insert(SectionId(0), section_header("core", None));
            map.insert(SectionId(1), section_header("core", None));
            map
        };
        assert_eq!(headers(&config), expected_separators);
        assert_eq!(config.next_section_id, 2);
        let expected_lookup_tree = {
            let mut tree = HashMap::new();
            tree.insert(
                section::Name("core".into()),
                SectionLookup {
                    without_subsection: vec![SectionId(0), SectionId(1)],
                    ..Default::default()
                },
            );
            tree
        };
        assert_eq!(config.section_lookup_tree, expected_lookup_tree);
        let expected_sections = {
            let mut sections = HashMap::new();
            sections.insert(
                SectionId(0),
                vec![
                    newline_event(),
                    name_event("a"),
                    Event::KeyValueSeparator,
                    value_event("b"),
                    newline_event(),
                    name_event("c"),
                    Event::KeyValueSeparator,
                    value_event("d"),
                    newline_event(),
                ],
            );
            sections.insert(
                SectionId(1),
                vec![name_event("e"), Event::KeyValueSeparator, value_event("f")],
            );
            sections
        };
        assert_eq!(bodies(&config), expected_sections);
        assert_eq!(config.section_order, [SectionId(0), SectionId(1)]);
    }

    #[test]
    fn plain_and_subsection_ids_have_one_direct_lookup() {
        let config = File::try_from(
            "[core] key=plain-1\n\
             [core \"a\"] key=a-1\n\
             [other] key=other\n\
             [core] key=plain-2\n\
             [core \"b\"] key=b\n\
             [core \"a\"] key=a-2\n",
        )
        .unwrap();
        let mut by_subsection = HashMap::new();
        by_subsection.insert("a".into(), vec![SectionId(1), SectionId(5)]);
        by_subsection.insert("b".into(), vec![SectionId(4)]);

        assert_eq!(
            &config.section_lookup_tree[&section::Name("core".into())],
            &SectionLookup {
                without_subsection: vec![SectionId(0), SectionId(3)],
                by_subsection,
            },
            "all `core` sections share one lookup, with file-ordered IDs separated by subsection"
        );
        assert_eq!(
            config.section_order,
            [
                SectionId(0),
                SectionId(1),
                SectionId(2),
                SectionId(3),
                SectionId(4),
                SectionId(5)
            ],
            "the global section order preserves interleaving across names and subsections"
        );
    }
}

fn headers(config: &crate::File) -> HashMap<SectionId, OwnedHeader> {
    config
        .sections
        .iter()
        .map(|(k, v)| (*k, own_header(&v.header, &config.backing)))
        .collect()
}

fn bodies(config: &crate::File) -> HashMap<SectionId, Vec<OwnedEvent>> {
    config
        .sections
        .iter()
        .map(|(k, v)| {
            (
                *k,
                v.body.0.iter().map(|event| own_event(event, &config.backing)).collect(),
            )
        })
        .collect()
}
