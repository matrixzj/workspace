#! /bin/bash
# From Matrix Zou for keep submit a directory to Git
# Maintainer    : Matrix Zou<jzou@freewheel.tv>
# Version       : "0.2-20160804"
# 
# ChangeLog	: 
# 	"0.21-20160806" -- modified files list generator bug fix
# 	"0.2-20160804" -- add / del files tracked
# 	"0.1-20160603" -- Initial script
#
# $1 = Directory
#
# crontab -u root -e
# 21 * * * *  /<path>/config2git.sh <Directroy>
#

die_msg() { echo ERROR: $@>&2; exit -1; }

[ ! -d $1 ] && die_msg Only accept a directory as parameter

cd $1 &>/dev/null || die_msg $0 cannot switch to working directory $1

project=`git remote -v | head -n1 | awk '{print $2}' | sed 's/.*\///' | sed 's/\.git//'`

modified_files() {
	modified_numbers=`git status -s | grep '^ M' | awk '{print $2}' | wc -l`
	if [ $modified_numbers == 0 ]; then
		echo "No files have been modified. " >> /tmp/${project}_commit_comment
		return 0
	else 
		echo "There are $modified_numbers files have been modified at $(date +\%Y-\%m-%d\ \%H:\%M:\%S). " >> /tmp/${project}_commit_comment
		git status -s | grep '^ M' | awk '{print $2}'  > /tmp/${project}_modifiled_filelist
		for file in `cat /tmp/${project}_modifiled_filelist`; do printf "\t%s\n" "$(ls -l $file)" >> /tmp/${project}_commit_comment; git add $file; done
	fi
}

add_files() {
        add_numbers=`git status -s | grep '^??' | awk '{print $2}' | wc -l`
        if [ $add_numbers == 0 ]; then
                echo "No files have been added. " >> /tmp/${project}_commit_comment
		return 0
        else
                echo "There are $add_numbers files have been added at $(date +\%Y-\%m-%d\ \%H:\%M:\%S). " >> /tmp/${project}_commit_comment
        	git status -s | grep '^??' | awk '{print $2}'  > /tmp/${project}_add_filelist
        	for file in `cat /tmp/${project}_add_filelist`; do printf "\t%s\n" "$(ls -l $file)" >> /tmp/${project}_commit_comment; git add $file; done
        fi
}

del_files() {
        del_numbers=`git status -s | grep '^\s*D' | awk '{print $2}' | wc -l`
        if [ $del_numbers == 0 ]; then
                echo "No files have been deleted. " >> /tmp/${project}_commit_comment
		return 0
        else
                echo "There are $del_numbers files have been deleted at $(date +\%Y-\%m-%d\ \%H:\%M:\%S). " >> /tmp/${project}_commit_comment
                git status -s | grep '^\s*D' | awk '{print $2}'  > /tmp/${project}_del_filelist
                for file in `cat /tmp/${project}_del_filelist`; do printf "\t%s\n" "$file" >> /tmp/${project}_commit_comment; git rm $file; done
        fi
}

lines=`git status -s | wc -l` 

if [ $lines != 0 ]; then
	modified_files
	add_files
	del_files
	git commit -F /tmp/${project}_commit_comment
	git push || die_msg $0 git commit failed
	rm -f /tmp/${project}_*
else
	exit 0 
fi
