#  Copyright (c) 2019 
#  All rights reserved. 
#  
#  文件名称:  bakHandle.sh
#  文件标识:  
#  			实现数据库和网站源代码的备份
#  
#  当前版本: 1.0 
#  作 者: tang.dong
#  开始日期: 2019/09/25
#  完成日期: 
#  其它说明： 
#  修改日期 版本号 修改人 修改内容 
#  ----------------------------------------------- 
#  2019/09/25 V1.0 汤栋 创建 
#  2019/10/21 V1.0 汤栋 去除压缩包中的多级目录 
# 

#!/bin/bash
#进入存放bakHandle.sh目录
cd ~/shellFiles/bakHandle

#需要备份的路径
htmlDir="/var/www/html"
mysqlDir="/var/lib/mysql"

#备份文件存放的路径
bakDir="$PWD/bak"
bakHtmlDir="$bakDir/bakhtml"
bakMysqlDir="$bakDir/bakMySQL"

#根据时间的压缩名
nowDay=$(date "+%Y-%m-%d-%H-%M-%S")
tarHtmlName="html$nowDay.tar.gz"
tarMySQLName="mysql$nowDay.tar.gz"

#过滤不要的文件
excludeFile=("app" \
			"Uploads" \
			"radiolist" \
			"all.tar.gz")

#########################################################

#备份html文件
function bakhtml(){
	if [ ! -d "$bakHtmlDir" ]; then
		mkdir $bakHtmlDir
	fi
	
	cd $htmlDir
	
	#拼接tar命令
	tarhtml="tar -czPf $bakHtmlDir/$tarHtmlName"
	#去除不需要压缩的文件
	for var in ${excludeFile[@]}
	do
		tarhtml="$tarhtml --exclude $var"
	done
	tarhtml="$tarhtml *"
	
	$tarhtml
	
	#mv "$PWD/$tarHtmlName" "$bakHtmlDir"
	rmOldFile $bakHtmlDir
}

#备份mysql文件
function bakMySQL(){	
	if [ ! -d "$bakMysqlDir" ]; then
		mkdir $bakMysqlDir
	fi

	cd $mysqlDir
	
	#拼接tar命令
	tarMySQL="tar -czPf $bakMysqlDir/$tarMySQLName *"
	
	$tarMySQL

	#mv "$PWD/$tarMySQLName" "$bakMysqlDir"
	rmOldFile $bakMysqlDir
}

#超过3个.tar.gz文件之后，删除最旧的一个.tar.gz文件 
#传入的数据$1为备份文件存放的地址
function rmOldFile(){
	cd $1
 	if [ $(ls -l | grep ".tar.gz" | wc -l) -gt 3 ]
 	then
     	echo "file > 3"
    	rm -rf $(ls -rt | grep ".tar.gz" | head -n1)
 	fi
}

function main(){
	if [ ! -d "$bakDir" ]; then
		mkdir $bakDir
	fi
	
	bakhtml
	bakMySQL
}

############################################################

main