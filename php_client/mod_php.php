<?php
$file_info = pfs_modify_file("61.143.251.62", 49711, "pfs_client.so", "pic", "/home/webadm/htdocs/photo2video/upImg//0/1/MTM2OTgwNzg2NF8wMDAwMA==.so");
if ($file_info)
{
	$group_name = $file_info['group'];
	echo "group: $group_name\n";
	$remote_filename = $file_info['remotefile'];
	echo "file:$remote_filename\n";
	var_dump($file_info);
}
else
	var_dump($file_info);
?>
