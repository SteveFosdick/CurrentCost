my $status = 0;
my $new = 'new.xml';
foreach my $file (@ARGV) {
    if (open(IN, '<', $file)) {
	if (open(OUT, '>', $new)) {
	    while (<IN>) {
		chomp;
		if (m/^(.*<host-tstamp>)(\d+)(<.*$)/oi) {
		    $_ = $1 . ($2 + 3600) . $3;
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
}
exit $status;
