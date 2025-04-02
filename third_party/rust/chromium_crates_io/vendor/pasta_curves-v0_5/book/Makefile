.PHONY: all
all:
	find src -type f -a -name '*.md' |sed 's/[.]md$$/.html/g' |xargs $(MAKE)

clean:
	find src -type f -a -name '*.html' -print0 |xargs -0 rm

%.html: %.md
	pandoc --katex --from=markdown --to=html "$<" "--output=$@"
	./edithtml.sh "$@" "$<"
