#!/usr/bin/env perl
use warnings;
use strict;
use v5.10; # for say
use File::Which;
use File::Find;
use File::Spec;
use File::Temp;
use Cwd;

# define some variables
my $project='hugin';
my $bugaddr='https://bugs.launchpad.net/hugin/';
my $copyright='Pablo dAngelo';

# safety checks
my $xgettext=which 'xgettext';
if(!defined $xgettext) {die 'ERROR: xgettext not found';}
my $wxrc=which 'wxrc';
if(!defined $wxrc) { die 'ERROR: wxrc not found'; }
my $msgmerge=which 'msgmerge';
if(!defined $msgmerge) { die 'ERROR: msgmerge not found'; }
# store current directory
my $basedir=getcwd;
say 'Found all necessary programs';

# create temp directory for all temporary files
my $temp_dir=File::Temp::tempdir(CLEANUP => 1);
# temp file which stores all files which should be searched by xgettext
my ($file_list_handle, $file_list)=File::Temp::tempfile(DIR => $temp_dir);
# store some absolute file names
my $path_potfiles_in=File::Spec->rel2abs('POTFILES.in', $basedir);
my $path_project_pot=File::Spec->rel2abs("$project.pot", $basedir);

# parse all xrc files with wxrc
chdir "..";
my @xrc_to_cpp;
find({wanted => sub { if (-f and /\.xrc$/)
  {
    my $filename=File::Spec->abs2rel($File::Find::name);
    say("Processing $filename");
    my $path_temp;
    (undef, $path_temp) = File::Temp::tempfile(DIR => $temp_dir, OPEN => 0);
    system($wxrc, '-g', $filename, '-o', $path_temp);
    push @xrc_to_cpp, $path_temp;
  }}, no_chdir => 1}, getcwd);

# extract all translatable strings from user defined files
my $translations_user_defined;
(undef, $translations_user_defined) = File::Temp::tempfile(DIR => $temp_dir, OPEN => 0);
find({wanted => sub { if (-f and /\.(assistant|executor)$/)
  {
    my $filename=File::Spec->abs2rel($File::Find::name);
    say("Processing $filename");
    system('perl', "$basedir/extract_user_defined.pl", $filename, $translations_user_defined);
  }}, no_chdir => 1}, getcwd);

#create array of all C++ files
my @cfiles;
my $cstartdir=getcwd;
find({wanted => sub {  if (-f) {
    my $filename=File::Spec->abs2rel($File::Find::name);
    if ($^O eq 'MSWin32') {$filename =~ tr#\\#/#;};
    push(@cfiles, $filename) if /\.(cpp|c|h)$/;
  }}, no_chdir => 1}, getcwd);
@cfiles=sort @cfiles;
push @cfiles, @xrc_to_cpp;
push @cfiles, $translations_user_defined;

# create input file list for xgettext
say 'Extracting messages';
# first write all C++ files in file
foreach (@cfiles) {say $file_list_handle $_;};
# then add files from POTFILES.in (e.g. tips)
open(my $potfilein, '<', $path_potfiles_in) or die "ERROR: Could not open file $path_potfiles_in";
while(<$potfilein>){say $file_list_handle $_;};
close $potfilein;
close $file_list_handle;

# run xgettext
chdir $basedir;
system($xgettext, '--from-code=UTF-8', '--c++', '--keyword=_', '--copyright-holder='.$copyright,
    '--msgid-bugs-address='.$bugaddr, '--files-from='.$file_list, '--directory='.$cstartdir, 
    '--output='.$path_project_pot)==0 or die "ERROR during running $xgettext";
say 'Done extracting messages';

# update all po files with new template
say 'Merging translations';
find(sub{if (-f and /\.po$/) {
    system($msgmerge, '--verbose', '-o', "$_.new.po", $_, $path_project_pot)==0 or die "ERROR during running $msgmerge";
    rename("$_.new.po", $_);
  }}, '.');
say 'Done merging translations';
# unify line breaks to unix
say 'Unify line breaks';
system('perl', 'unix_linebreaks.pl', $path_project_pot, '*.po', 'outdated/*.po');
say 'All done';
