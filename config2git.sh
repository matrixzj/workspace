#! /bin/bash
# From Matrix Zou for keep submit a directory to Git
# Maintainer    : Matrix Zou<matrix.zj@gmail.com>
# Version       : "0.2-20160804"
#
# ChangeLog     :
#       "0.2-20160804" -- add / del files tracked
#       "0.1-20160603" -- Initial script
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
	modified_numbers=`git status -s | grep '^\s*M' | awk '{print $2}' | wc -l`
	if [ $modified_numbers == 0 ]; then
		echo "No files have been modified. " >> /tmp/${project}_commit_comment
		return 0
	else 
		printf "There are %s files have been modified at %s .\n" "$modified_numbers" "$(date +%Y-%m-%d\ %H:%M:%S)" >> /tmp/${project}_commit_comment
		# echo "There are $modified_numbers files have been modified at $(date +\%Y-\%m-%d\ \%H:\%M:\%S). " >> /tmp/${project}_commit_comment
		git status -s | grep '^\s*M' | awk '{print $2}'  > /tmp/${project}_modifiled_filelist
		for file in `cat /tmp/${project}_modifiled_filelist`; do printf "\t%s\n" "$(ls -l $file)" >> /tmp/${project}_commit_comment; git add "$file"; done
	fi
}

add_files() {
        add_numbers=`git status -s | grep '^??' | awk '{print $2}' | wc -l`
        if [ $add_numbers == 0 ]; then
                echo "No files have been added. " >> /tmp/${project}_commit_comment
		return 0
        else
                echo "There are $add_numbers files have been added at $(date +\%Y-\%m-%d\ \%H:\%M:\%S). " >> /tmp/${project}_commit_comment
        	git status -s | grep '^??' | grep -v '/$' | awk '{print $2}'  > /tmp/${project}_add_filelist
        	for file in `cat /tmp/${project}_add_filelist`; do printf "\t%s\n" "$(ls -l $file)" >> /tmp/${project}_commit_comment; git add $file; done
		# add a folder
		git status -s | grep '^??' | grep '/$' | awk '{print $2}'  > /tmp/${project}_add_dir
		for dir in `cat /tmp/${project}_add_dir`; do printf "\t%s\n" "$(ls -ld $dir)" >> /tmp/${project}_commit_comment; git add "$dir"; done
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
                for file in `cat /tmp/${project}_del_filelist`; do printf "\t%s\n" "$file" >> /tmp/${project}_commit_comment; git rm "$file"; done
        fi
}

change_files_type() {
        chg_numbers=`git status -s | grep '^\s*T' | awk '{print $2}' | wc -l`
        if [ $chg_numbers == 0 ]; then
                echo "No file type has been changed. " >> /tmp/${project}_commit_comment
                return 0
        else
                echo "There are $chg_numbers file types have been changed at $(date +\%Y-\%m-%d\ \%H:\%M:\%S). " >> /tmp/${project}_commit_comment
                git status -s | grep '^\s*T' | awk '{print $2}'  > /tmp/${project}_chg_filelist
                for file in `cat /tmp/${project}_chg_filelist`; do printf "\t%s\n" "$file" >> /tmp/${project}_commit_comment; git add "$file"; done
        fi
}


lines=`git status -s | wc -l` 

if [ $lines != 0 ]; then
	rm -f /tmp/${project}_*
	modified_files $1
	add_files $1
	del_files $1
	change_files_type $1
	git commit -F /tmp/${project}_commit_comment >/dev/null 2>&1
        git push >/dev/null 2>&1
        if [ $? -ne 0 ]; then
                die_msg $0 git commit failed
        else
                echo "${project} git committed."
        fi
        cat /tmp/${project}_commit_comment
        rm -f /tmp/${project}_*
else
	exit 0 
fi
