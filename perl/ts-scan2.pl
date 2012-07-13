use POSIX qw(strftime);

my $prev = 0;
foreach my $file (@ARGV) {
    print "$0: $file\n";
    if (open(IN, '<', $file)) {
	while (<IN>) {
	    chomp;
	    if (m/<host-tstamp>(\d+)</oi) {
		my $secs = $1;
		if ($secs < $prev) {
		    print "backwards at line $., time ", strftime('%Y-%m-%d %H:%M:%S', gmtime($secs)), "\n";
		}
		$prev = $secs;
	    }
	}
	close(IN);
    }
}
