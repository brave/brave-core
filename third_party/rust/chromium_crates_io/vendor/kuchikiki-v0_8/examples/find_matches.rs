use kuchikiki::traits::TendrilSink;

fn main() {
    let html = r"
        <!DOCTYPE html>
        <html>
        <head></head>
        <body>
            <h1>Example</h1>
            <p class='foo'>Hello, world!</p>
            <p class='foo'>I love HTML</p>
        </body>
        </html>
    ";
    let css_selector = ".foo";

    let document = kuchikiki::parse_html().one(html).document_node;

    for css_match in document.select(css_selector).unwrap() {
        // css_match is a NodeDataRef, but most of the interesting methods are
        // on NodeRef. Let's get the underlying NodeRef.
        let as_node = css_match.as_node();

        // In this example, as_node represents an HTML node like
        //
        //   <p class='foo'>Hello world!</p>"
        //
        // Which is distinct from just 'Hello world!'. To get rid of that <p>
        // tag, we're going to get each element's first child, which will be
        // a "text" node.
        //
        // There are other kinds of nodes, of course. The possibilities are all
        // listed in the `NodeData` enum in this crate.
        let text_node = as_node.first_child().unwrap();

        // Let's get the actual text in this text node. A text node wraps around
        // a RefCell<String>, so we need to call borrow() to get a &str out.
        let text = text_node.as_text().unwrap().borrow();

        // Prints:
        //
        //  "Hello, world!"
        //  "I love HTML"
        println!("{:?}", text);
    }
}
