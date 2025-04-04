#!/bin/sh

if [ ! -d themes/hugo-geekdoc ]; then
	mkdir -p _deps || true
	mkdir -p themes/hugo-geekdoc || true
	if [ ! -f _deps/hugo-geekdoc.tar.gz ]; then
		wget -O _deps/hugo-geekdoc.tar.gz https://github.com/thegeeklab/hugo-geekdoc/releases/latest/download/hugo-geekdoc.tar.gz
	fi
	tar xzvfC _deps/hugo-geekdoc.tar.gz themes/hugo-geekdoc
fi

hugo --cleanDestinationDir
