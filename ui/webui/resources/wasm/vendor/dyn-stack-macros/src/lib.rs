use proc_macro::*;

#[proc_macro]
pub fn alloc_impl_proc(item: TokenStream) -> TokenStream {
	let mut item = item.into_iter();
	let Some(TokenTree::Group(krate)) = item.next() else { panic!() };
	let krate = krate.stream();
	let stack = match item.next() {
		Some(TokenTree::Group(stack)) => {
			let mut stack = stack.stream().into_iter();
			_ = stack.next();
			let Some(TokenTree::Ident(stack)) = stack.next() else { panic!() };
			stack
		},
		Some(_) => {
			let Some(TokenTree::Ident(stack)) = item.next() else { panic!() };
			stack
		},

		_ => panic!(),
	};
	let Some(TokenTree::Group(block)) = item.next() else { panic!() };

	let mut tokens = Vec::new();

	tokens.extend([
		TokenTree::Ident(Ident::new("let", Span::call_site())),
		TokenTree::Ident(stack.clone()),
		TokenTree::Punct(Punct::new('=', Spacing::Alone)),
		TokenTree::Ident(stack.clone()),
		TokenTree::Punct(Punct::new(';', Spacing::Alone)),
	]);

	let mut cur_stmt = vec![TokenTree::Ident(stack.clone())];
	for token in block.stream().into_iter() {
		match token {
			TokenTree::Punct(p) if p.as_char() == ';' => {
				tokens.extend(krate.clone().into_iter().chain([
					TokenTree::Punct(Punct::new(':', Spacing::Joint)),
					TokenTree::Punct(Punct::new(':', Spacing::Alone)),
					TokenTree::Ident(Ident::new("alloc_impl_rules", Span::call_site())),
					TokenTree::Punct(Punct::new('!', Spacing::Alone)),
					TokenTree::Group(Group::new(Delimiter::Parenthesis, TokenStream::from_iter(cur_stmt))),
					TokenTree::Punct(Punct::new(';', Spacing::Alone)),
				]));

				cur_stmt = vec![TokenTree::Ident(stack.clone())];
			},
			token => cur_stmt.push(token),
		}
	}

	tokens.extend([
		TokenTree::Ident(Ident::new("let", Span::call_site())),
		TokenTree::Ident(stack.clone()),
		TokenTree::Punct(Punct::new('=', Spacing::Alone)),
		TokenTree::Ident(stack.clone()),
		TokenTree::Punct(Punct::new(';', Spacing::Alone)),
	]);

	TokenStream::from_iter(tokens)
}
