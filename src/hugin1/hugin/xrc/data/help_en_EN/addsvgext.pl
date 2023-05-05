#!/usr/bin/perl

use strict;
use warnings;

# SVG files are retrieved without .svg extension, which breaks locally served web pages

for my $file (@ARGV)
{
    next unless $file =~ /^[a-f0-9]{40}$/;
    rename $file, "$file.svg";
    system ('rsvg-convert', '-o', "$file.png", "$file.svg");
    unlink ("$file.svg");
}
