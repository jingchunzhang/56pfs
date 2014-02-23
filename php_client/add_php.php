<?php
$file_info = pfs_add_file("10.16.84.235:49711", "photo", "pfs_add.jpg", 0);
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

$file_info = pfs_add_file("10.16.84.235:49711", "p1.pfs.56img.com", "pfs_add.jpg", 1);
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

$file_info = pfs_add_file("10.16.84.235:49711", "photo", "pfs_add.jpg", 2);
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

$file_info = pfs_add_file("10.16.84.235:49711", "p1.pfs.56img.com", "pfs_add.jpg", 3);
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
