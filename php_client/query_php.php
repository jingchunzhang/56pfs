<?php
$file_info = pfs_query_other("10.16.84.242", "picxxx", 5);

if ($file_info)
{
	$retcode = $file_info['retcode'];
	if ($retcode === 0)
	{
		echo "OK!\n";
		var_dump($file_info);
	}
	else
	{
		var_dump($file_info);
	}
}
?>
