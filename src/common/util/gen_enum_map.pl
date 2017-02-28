#!/usr/bin/env perl
#

use strict;
use warnings;

sub extract_enums {
    my ($fname) = @_;
    my $in = 0;

    open FH, "<", $fname or die "$!";
    my $content = do { local $/; <FH>; };
    close FH;
    my (%enums) = ($content =~ /^\s*[^\/]enum\s*([^\s{]*?)\s*{([^}]*)/msg);

    for(keys %enums) {
	$enums{$_} =~ s/\/\/.*//g; 
	$enums{$_} =~ s/\s//g; 
	my @r = split /,/, $enums{$_};
	$enums{$_} = \@r;
    }

    return %enums;
}

sub gen_strarr {
    my (%enums) = @_;
    for(keys %enums) {
	next if($_ eq "");
	print HFH "extern char *${_}2str[];\n";
	print CFH "char *${_}2str[] = {\n";
	print CFH "\t\"$_\",\n" for(@{$enums{$_}});
	print CFH "};\n\n";
    }

}

my $usage = "$0 <header file> [<header file> ...]";
die $usage unless @ARGV;

open CFH, ">", "enum_map.c";
open HFH, ">", "enum_map.h";

print HFH "#ifndef _ENUM_MAP_H_\n";
print HFH "#define _ENUM_MAP_H_\n\n";
for(@ARGV) {
    my %enums = extract_enums $_;
    gen_strarr(%enums);
}

print HFH "#endif\n";
close CFH;
close HFH;
