#!/usr/bin/perl

use Cwd;
use File::Copy;

my $dir = getcwd;
my @files = ();

# Delete any existing PCI_LA.pciacq file
unlink("PCI_LA.pciacq");

printf "\n\nFound the following PCI Acquisition files in $dir";
printf "\n\n";
opendir (DIR, $dir) or die $!;

my $i = 1;
while (my $file = readdir(DIR)) {
	if ( $file =~ m/pciacq/) {
		print "$i = $file\n";
		push(@files, $file);
		$i++;
	}
}
closedir(DIR);
print "Q - Quit";
printf "\n\nSelect the file you would like to see:";
$option = <>;
chomp($option);
if ( $option =~ m/q|Q/ ) {
	printf "\nExiting";
	exit(0);
}
printf "\nyou chose $option = $files[$option-1]";


# Copy selected file to that name
copy("$files[$option-1]", "PCI_LA.pciacq");

# Start PCIBusViewer.exe
system("./PCIBusViewer.exe");

# Delete copied file - PCI_LA.pciacq
unlink("PCI_LA.pciacq");
