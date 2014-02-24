```
/*
*	Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com; danezhang77 AT gmail.com; xiaojie.zhang1 AT renren-inc.com;
*	
*	56VFS may be copied only under the terms of the GNU General Public License V
*/
```


#pfs接口说明文档(php)

pfs按文件类型，有3个分组`group`  
1. 缩略图，group为`pic`  
2. 相册视频图片，group为`photo`  
3. 音乐mp3，group为`mp3`  

负责上传调度的服务端域名为`upload.p.56.com`


##pfs有4个接口
###1. 新增文件接口：pfs_add_file
####输入4个参数:  
1. 上传调度的服务端`nameserver:port`: namesever + port  （如果不带port参数， 默认端口49711）  
2. 分组`group`: 按文件类型对应一个字符串，为`pic`或`photo`或`mp3`之一  
3. 文件路径`localfile`: 待上传的本地文件路径  
4. 标志位`flag`，指定如下值:  
    `0`: group为文件分组名`pic`/`photo`/`mp3`3者之一，由pfs选择`实际存储的服务器`  
	`1`: group为`实际存储的服务器的域名`，开发人员自己选择一个`实际存储的服务器`，开发人员可以先调用`pfs查询接口pfs_query_other`来获取一个存储服务器域名，再调用`pfs_add_file`上传文件    
	`2`: 上传文件名为本地文件名(basename)，若指定为2，pfs就不会对图片名称做base64编码  
	`3`: 结合了`1&2`的操作
	
####返回参数:
返回一个array，包含retcode（M），domain（O），remotefile（O），retmsg（O）  
Retcode = 0时，上传正确，返回上传存储位置domain + remotefile  
Retcode != 0时，上传错误，retmsg包含具体错误信息

####添加demo：flag的使用
```
<?php
# 方式1
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

# 方式2
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

# 方式3
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

# 方式4
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
```  
	

###2. 删除文件接口：pfs_remove_file
####输入3参数：
1. `nameserver:port`: namesever + port  （如果不带port参数，默认端口49711）
2. `domain`: 存储分组，为`pic`或`photo`或`mp3`之一
3. `remotefile`: 待删除的远端文件

####返回参数
返回一个array，包含retcode（M），retmsg（M）  
Retcode = 0时，删除正确  
Retcode != 0时，删除错误，retmsg包含具体错误信息  

####删除demo
```
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
```


###3. 修改文件接口：pfs_modify_file
**修改文件接口也可作为新增文件接口，指定域名及后端存储路径**
####输入3个参数
1. `nameserver:port`: namesever + port  （如果不带port参数，默认端口49711）
2. `domain`: 存储分组，为`pic`或`photo`或`mp3`之一
3. `localfile`: 本地文件，新的文件
4. `remotefile`: 远端文件，将会被覆盖

####返回参数
返回一个array，包含retcode（M），domain（O），remotefile（O），retmsg（O）  
Retcode = 0时，修改正确，返回上传存储位置domain + remotefile  
Retcode != 0时，修改错误，retmsg包含具体错误信息

####demo
```
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
```


###4. 存储位置查询接口：pfs_query_other
####输入3个参数
1. `nameserver:port`: namesever + port  （如果不带port参数，默认端口49711）
2. `domain`: 存储分组，为`pic`或`photo`或`mp3`之一
3. `flag`: 取值为5，查询存储位置

####返回参数
返回一个array，包含retcode（M），domain（O），remotedir（O），errmsg（O）  
Retcode = 0时，上传正确，返回上传存储位 置domain + remotedir  
Retcode != 0时，上传错误，errmsg包含具体错误信息  
####demo
```
<?php
$file_info = pfs_query_other("upload.p.56.com", "pic", 5);

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
```