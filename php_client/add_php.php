<?php
$file_info = pfs_add_file("nameserver:port", "photo", "pfs_client.so");
if ($file_info)
{
	$retcode = $file_info['retcode'];
	if ($retcode === 0)
	{
		$domain_name = $file_info['domain'];
		echo "domain: $domain_name\n";
		$remote_filename = $file_info['remotefile'];
		echo "file:$remote_filename\n";
	}
	else
	{
		$errmsg = $file_info['retmsg'];
		echo "errmsg: $errmsg\n";
	}
}
?>
