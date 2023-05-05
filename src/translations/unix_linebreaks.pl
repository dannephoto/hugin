#!/usr/bin/env perl
use warnings;
use strict;

die 'No file given' unless scalar @ARGV>0;
my @allFiles;
while(my $f=shift @ARGV)
{
  push @allFiles, glob $f;
};
die 'No matching file found' unless scalar @allFiles>0;

foreach my $file (@allFiles)
{
  print "Processing $file\n";
  open(my $inputFile, '<', $file) or next;
  binmode $inputFile;
  open(my $outputFile, '>', "$file.new") or next;
  binmode $outputFile; 
  while(<$inputFile>){ s/\cM//g; print $outputFile $_;};
  close $inputFile;
  close $outputFile;
  rename("$file.new", "$file");
};
