eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id: run_test.pl 5502 2012-04-12 19:08:14Z johnsonb $
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use File::Basename;
use Cwd 'abs_path';
use strict;

my $portWriter = "\\\\.\\CNCA0";
my $portReader = "\\\\.\\CNCB0";

if ($#ARGV == 1) {
	$portWriter = $ARGV[0];
	$portReader = $ARGV[1];
}

# must write to the first port, read from the second port
#$portWriter = "/dev/pts/4";
#$portReader = "/dev/pts/6";

my $dirname = dirname(__FILE__);
my $dataPath = abs_path(File::Spec->catfile($dirname, "gps_2013-01-15_0106"));
print $dataPath . "\n";

my $status = 0;
my $common_opts = "-ORBDebugLevel 10 -DCPSDebugLevel 10";

# my $pub_opts = "$common_opts -ORBLogFile publisher.log -p \\\\.\\COM16 -b 4800";
my $writer_opts = "-p " . $portWriter;
$writer_opts = $writer_opts . " -b 4800 -f $dataPath";  # -n 

my $pub_opts = "-p " . $portReader;
$pub_opts = $pub_opts . " -b 4800";
$pub_opts = $pub_opts . " $common_opts -ORBLogFile publisher.log ";

my $sub_opts = "$common_opts -DCPSTransportDebugLevel 6 " .
               "-ORBLogFile subscriber.log";

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                        "-ORBDebugLevel 10 " .
                                        "-ORBLogFile DCPSInfoRepo.log " .
                                        "-o $dcpsrepo_ior");

my $Writer = PerlDDS::create_process ("SerialWriter", " $writer_opts");
my $Subscriber = PerlDDS::create_process ("GPSSubscriber", " $sub_opts");
my $Publisher = PerlDDS::create_process ("GPSPublisher", " $pub_opts");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}
if ($portWriter ne "none") {
    print $Writer->CommandLine() . "\n";
    $Writer->Spawn ();
}

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();

my $PublisherResult = $Publisher->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

my $WriterResult = $Writer->WaitKill (300);
if ($WriterResult != 0) {
    print STDERR "ERROR: writer returned $WriterResult \n";
    $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill (300);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

my $ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;

exit $status;
