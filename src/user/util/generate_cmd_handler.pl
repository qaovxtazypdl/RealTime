#!/usr/bin/env perl

use strict;
use utf8;

my $delegator_function = "handle_cmd";

# Generate prototypes for the functions expected by the internal arg handlers (this is the entry point to your code)

sub generate_handler_prototypes {
  my %cmd_map = @_;
  my $res = "";

  $res .= "";

  foreach my $c (keys %cmd_map) {
    $res .= "void handle_${c}_cmd(";
    my $argnum = 0;

    if(@{$cmd_map{$c}} > 0) {
      for (@{$cmd_map{$c}}) {
        $argnum++;
        if($_ eq "int") {
          $res .= "int v$argnum, ";
        } else {
          $res .= "char *v$argnum, ";
        }
      }

      $res =~ s/..$//;
    }
    $res .= ");\n";
  }
  return $res;
}

# Consumes a list of commands and generates prototypes for the internal arg handlers as well as the main handler map

sub generate_header {
  my %cmd_map = @_;
  my $res = "";

  $res .= "#define CMD_INVALID_ARG_STR 1\n";
  $res .= "#define CMD_BAD_ARG_SIZE 2\n";
  $res .= "#define CMD_BAD_ARG_TYPES 3\n";
  $res .= "#define CMD_NO_HANDLER_FOUND -1\n";

  $res .= "int $delegator_function(char *cmd_str);\n\n";
  $res .= "//The generated code expects the below functions to be implemented and consistent with the generated prototypes, this is the main entry point for your code\n\n";
  $res .= generate_handler_prototypes(%cmd_map);

  return $res;
}

#TODO add error handling for bad/short/long input

# Generate the internal argument handler (one which consumes a raw arg string, 
# parses it, and then called the appropriate user defined function)

sub generate_internal_arg_handler {
  my $res = "";
  my ($cmd, @types) = @_;

  $res .=("static int32_t handle_argstr_$cmd(char *argstr) {\n");
  $res .= "	char *token;\n";
  $res .= "	byte err;\n";
  my $argnum = 0;
  for(@types) {
    $argnum++;
    if($_ eq "int") {
      $res .= "	int v$argnum;\n";
    } else {
      $res .= "	char *v$argnum;\n";
    }
  }

  $res .= "\n";

  $argnum = 0;
  for(@types) {
    $argnum++;
    if($_ eq "int") {
      $res .= "	token = get_token(&argstr);\n";
      $res .= "	if(token == NULL) return CMD_BAD_ARG_SIZE;\n\n";
      $res .= "	v$argnum = strtoi(token, 1, &err);\n";
      $res .= "	if(err) return CMD_BAD_ARG_TYPES;\n\n";
    } else {
      $res .= "	v$argnum = get_token(&argstr);\n";
      $res .= "	if(v$argnum == NULL) return CMD_BAD_ARG_SIZE;\n\n";
    }
  }

  $res .= "\tif(get_token(&argstr)) return CMD_BAD_ARG_SIZE;\n\n";
  $res .= "	handle_${cmd}_cmd(";

  my $args = "";
  $argnum = 0;
  for(@types) {
    $argnum++;
    $args .= "v$argnum, ";
  }
  $args =~ s/..$//;

  $res .= "$args);\n";
  $res .= "\treturn 0;\n";
  $res .= "}\n";

  return $res;
}	


sub generate_main_c_file {
  my ($header_file, %cmd_map) = @_;
  my $res = <<"!";
#include <gen/cmd.h>
#include <common/string.h>
!

    for(keys %cmd_map) {
      print "Generating internal argument parser for $_\n";
      $res .= generate_internal_arg_handler($_, @{$cmd_map{$_}});
    }

  $res .= "int $delegator_function(char *cmd_str) {\n";
  $res .= "\tchar *cmd = get_token(&cmd_str);";
  $res .= "if(!streq(\"$_\", cmd))\n\treturn handle_argstr_$_(cmd_str);\n" for (keys %cmd_map);
  $res .= "return CMD_NO_HANDLER_FOUND;\n}";

  return $res;
}

my $usage = "$0 <spec file> <output_base>";
die $usage unless @ARGV == 2;
my $spec_file = shift;
my $base_file_name = shift;

my $header_file = "${base_file_name}.h";
my $output_file = "${base_file_name}.c";

# Mapping of commamds to their type lists
my %cmd_map = ();

open FH, "<", $spec_file;
while(<FH>) {
  chomp;
  my ($cmd, @args) = split /\s*,\s*/;
  $cmd_map{$cmd} = \@args;
}
close FH;

open FH, ">", $header_file;
print FH "// GENERATED CODE DO NOT EDIT (edit $spec_file and rerun $0)\n\n";
print FH "#ifndef _HANDLERS_H_\n";
print FH "#define _HANDLERS_H_\n\n";
print FH generate_header %cmd_map;
print FH "#endif\n";
close FH;

open FH, ">", $output_file;
print FH "// GENERATED CODE DO NOT EDIT (edit $spec_file and rerun $0)\n\n";
print FH generate_main_c_file($header_file, %cmd_map);
close FH;
