use POSIX qw(strftime);

foreach my $file (@ARGV) {
    print "$0: $file\n";
    if (open(IN, '<', $file)) {
	while (<IN>) {
	    chomp;
	    if (m/<host-tstamp>(\d+).*<time>([\d:]+)</oi) {
		my $secs = $1;
		print $., ' ',
		strftime('%Y-%m-%d %H:%M:%S', gmtime($secs)), " $2\n";
	    }
	}
	close(IN);
    }
}
