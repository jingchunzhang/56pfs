<?php
$file_info = pfs_modify_file("10.16.84.235:49711", "p1.pfs.56img.com", "pfs_modify.jpg", "0/3/pfs_modify_on_storage.jpg");
if ($file_info)
{
	$domain_name = $file_info['domain'];
	echo "domain: $domain_name\n";
	$remote_filename = $file_info['remotefile'];
	echo "file:$remote_filename\n";
	var_dump($file_info);
}
?>
