#
# Configuration file reader
# Wang Lam <wlam@cs.stanford.edu>
# January 2000
# Last updated 30 Mar 2001, to implement "include".
#
# Usage:
#    require "confloader.pl";
#    &ConfLoader::loadValues('WebBase-configuration-filename');
#    $flinks_dir = &ConfLoader::getValue('FLINKS_DIR');
# 
# Largely emulates C++ confloader behavior, assuming '=' as the property/value
# delimiter.
#
package ConfLoader;

# No configuration file loaded
my $loaded = 0;
my %PROPERTY = ();

# ConfLoader::loadValues(filename)
#    Reads the contents of a configuration file.
#    Returns 0 on failure, 1 on success.
#
sub loadValues {
   local *CONFFILE;
   open(CONFFILE,shift @_) || return 0;
   while(<CONFFILE>) {
      next if /^\s*#/;  # Toss entire-line comments
      if (/^include /) {
         my($file) = /^include (.*)$/;
         &ConfLoader::loadValues($file);
      };
      next unless /=/;  # Ignore lines that don't have the prop/val delimiter
                        # (Emulates C++ version confloader behavior)
      chomp;
#     my ($string,$comment) = split('#',$_,2);
                        # Allow end-of-line comments (NOT in C++ version)
      my $string = $_;
      my ($prop,$val) = $string =~ /^([^=]*)=(.*)$/;
                        # (Emulates C++ version behavior) split on first =
      $prop =~ s/\s//g;  $val =~ s/\s//g;
                        # whitespace now aggressively destroyed (emulates C++)
      $PROPERTY{$prop} = $val;  # whitespace is not eliminated (emulates C++)
   }
   close(CONFFILE);
   $loaded = 1;
   return 1;
}

# ConfLoader::getValue(propertyname)
#    Gets the value of the given property.
#    Returns undef if a property with given name is not found, or if
#       ConfLoader::loadValues has not been called successfully before.
#
sub getValue {
   return undef unless $loaded;
   return $PROPERTY{@_[$[]};
}

1;  # in case require or somesuch worries that this package return true
