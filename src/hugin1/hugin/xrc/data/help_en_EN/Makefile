# downloads a selection of pages specified in the 'pages.txt' file from the
# Panotools wiki at http://wiki.panotools.org/ and assembles them into an
# offline HTML manual for Hugin.
#
# Requires wget and perl
#
# Bruno Postle <bruno@postle.net>

all: clean fetch dewiki

clean:
	mv license.html __TMP
	rm -f *.html
	mv __TMP license.html
	rm -f *.jpg *.JPG
	rm -f *.png *.PNG
	rm -f *.gif *.GIF
	rm -f *.svg

fetch: pages.txt
	wget --input-file=pages.txt \
	     --base=http://wiki.panotools.org/ \
	     --no-host-directories \
	     --html-extension \
	     --adjust-extension \
	     --page-requisites \
	     --convert-links \
	     --span-hosts \
	     --no-directories \
	     --exclude-directories=wiki/skins \
	     --ignore-tags=link
	rm -f robots.txt
	rm -f index.php*
	rm -f gnu-fdl.png
	rm -f load.php*
	rm -f poweredby_mediawiki_*.png

dewiki:
	perl dewiki.pl *.html
	perl addsvgext.pl *

.PHONY: clean fetch dewiki

