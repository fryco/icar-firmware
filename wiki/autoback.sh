#!/bin/bash
# Name:autoback.sh
# This is a ShellScript For Auto Backup and Delete old Backup
#

BACKUPWEBDIR=/var/chroot/home/content/72/8356172/backup/web
BACKUPBLOGDIR=/var/chroot/home/content/72/8356172/backup/blog
BACKUPCN0086WIKIDIR=/var/chroot/home/content/72/8356172/backup/cn0086wiki
#TIME=` date +%Y%m%d%H `
TIME=` date +%Y%m%d `

#backup the web site
echo "Backup  web site ..."
tar -jcvf $BACKUPWEBDIR/$TIME.bz2 /var/chroot/home/content/72/8356172/html
chmod 600 $BACKUPWEBDIR/$TIME.bz2

#backup the cn0086wiki datebase
echo "Backup wiki database ..."
mysqldump -h 182.50.131.193 -ucn0086wiki -ppassword cn0086wiki > /var/chroot/home/content/72/8356172/backup/cn0086wiki.sql

tar -jcvf $BACKUPCN0086WIKIDIR/$TIME.bz2 /var/chroot/home/content/72/8356172/backup/cn0086wiki.sql
chmod 600 $BACKUPCN0086WIKIDIR/$TIME.bz2

#backup the blog datebase
echo "Backup blog database ..."
mysqldump -h 182.50.131.232 -ucn01125806132081 -ppassword cn01125806132081 > /var/chroot/home/content/72/8356172/backup/cn01125806132081.sql

tar -jcvf $BACKUPBLOGDIR/$TIME.bz2 /var/chroot/home/content/72/8356172/backup/cn01125806132081.sql
chmod 600 $BACKUPBLOGDIR/$TIME.bz2

#Remove old file
echo "Remove old file ..."
find $BACKUWEBPDIR -name "*.bz2" -type f -mtime +30 -exec rm {} \; > /dev/null 2>&1
find $BACKUPCN0086WIKIDIR -name "*.bz2" -type f -mtime +30 -exec rm {} \; > /dev/null 2>&1
find $BACKUPBLOGDIR -name "*.bz2" -type f -mtime +30 -exec rm {} \; > /dev/null 2>&1
