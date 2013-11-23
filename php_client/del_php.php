<?php
$file_info = pfs_remove_file("nameserver:port", "domain", "/d1/d2/remotefile");

if ($file_info)
{
	$retcode = $file_info['retcode'];
	if ($retcode === 0)
	{
		echo "OK!\n";
	}
	else
	{
		$errmsg = $file_info['retmsg'];
		echo "errmsg: $errmsg\n";
	}
}
?>
