#!/usr/bin/env perl
use warnings;
use strict;

die 'Syntax extract_user_defined.pl input_file output_file' unless scalar @ARGV == 2;

open(my $inputFile, '<', $ARGV[0]) or die;
open(my $outputFile, '>>', $ARGV[1]) or die;

my $filename=$ARGV[0];
if ($^O eq 'MSWin32') {$filename =~ tr#\\#/#;};
my $line=1;
while(<$inputFile>){ 
  chomp;
  # split at =
  my @strings=split /=/;
  if(scalar @strings == 2)
  {
    if ($strings[0] eq 'Description' or $strings[0] eq 'Help')
    {
      $strings[1] =~ s/^\s+|\s+$//g;  # trim white spaces
      print $outputFile "#line $line \"$filename\"\n";
      print $outputFile "_(\"$strings[1]\")\n";
    };
  };
  $line++;
};
close $inputFile;
close $outputFile;
