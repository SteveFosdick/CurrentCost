#! /usr/bin/perl

use POSIX qw(mktime);

my $status = 0;
my $new = 'new.xml';
foreach my $file (@ARGV) {
    if (open(IN, '<', $file)) {
	if (open(OUT, '>', $new)) {
	    while (<IN>) {
		chomp;
		while (m/^(.*?<host-tstamp>)(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})Z?(<.*)$/) {
		    $_ = $1 . mktime($7, $6, $5, $4, $3-1, $2-1900) . $8;
		}
		print OUT "$_\n";
	    }
	    close(OUT);
	    unless (rename($new, $file)) {
		print STDERR "$0: unable to rename '$new' to '$file' - $!\n";
		$status = 3;
		last;
	    }
	} else {
	    print STDERR "$0: unable to open file '$new' for writing - $!\n";
	    $status = 2;
	}
	close(IN);
    } else {
	print STDERR "$0: unable to open file '$file' for reading - $!\n";
	$status = 1;
    }
    exit $status;
}
		    
