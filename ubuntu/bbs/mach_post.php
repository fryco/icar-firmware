<?php

/**
 *      [Discuz!] (C)2001-2099 Comsenz Inc.
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $Id$
 */
require './source/class/class_core.php';
require './source/function/function_forum.php';

if(!defined('IN_DISCUZ')) {
        define('IN_DISCUZ', true);
}

if(!defined('IN_UC')) {
        define('IN_UC', true);
}


C::app()->cachelist = $cachelist;
C::app()->init();

/* For user login, no need now
define('APPTYPEID', 0);
define('CURSCRIPT', 'member');

$discuz = C::app();

$_POST[action] = "login";

$mod = "logging";

define('CURMODULE', $mod);

$discuz->init();
if($mod == 'register' && $discuz->var['mod'] != $_G['setting']['regname']) {
	showmessage('undefined_action');
}

require libfile('function/member');
require libfile('class/member');
runhooks();

require DISCUZ_ROOT.'./source/module/member/mach_'.$mod.'.php';
*/

//echo $_POST["subject"];
//echo "<br>";
//echo $_POST["message"];

$fid = "36";
$publishdate = $_G['timestamp'];
$author = "machine";
$authorid = "2";

	$newthread = array(
		'fid' => $fid,
		'posttableid' => 0,
		'readperm' => 0,
		'price' => 0,
		'typeid' => 0,
		'sortid' => 0,
		'author' => $author,
		'authorid' => $authorid,
		'subject' => $_POST["subject"],
		'dateline' => $publishdate,
		'lastpost' => $publishdate,
		'lastposter' => $author,
		'displayorder' => 0,
		'digest' => 0,
		'special' => 0,
		'attachment' => 0,
		'moderated' => 0,
		'status' => 32,
		'isgroup' => 0,
		'replycredit' => 0,
		'closed' =>  0
	);

	$tid = C::t('forum_thread')->insert($newthread, true);
	useractionlog($authorid, 'tid');

	C::t('common_member_field_home')->update($authorid, array('recentnote'=>$_POST["subject"]));
	
	$pid = insertpost(array(
			'fid' => $fid,
			'tid' => $tid,
			'first' => '1',
			'author' => $author,
			'authorid' => $authorid,
			'subject' => $_POST["subject"],
			'dateline' => $publishdate,
			'message' => $_POST["message"],
			'useip' => '192.168.0.1',
			'invisible' => '0',
			'anonymous' => '0',
			'usesig' => '1',
			'htmlon' => '0',
			'bbcodeoff' => '-1',
			'smileyoff' => '-1',
			'parseurloff' => '0',
			'attachment' => '0',
			'tags' => '',
			'replycredit' => '0',
			'status' => '0'
	));


	include_once libfile('function/stat');
	updatestat("thread");

	//update user status
	$sql = array(
			'extcredits2' => 2,
			'threads' => 1,
			'posts' => 1
	);
	C::t('common_member_count')->increase($authorid, $sql);
	
	C::t('common_member')->increase($authorid, array('credits' => 2));
	
	$last_status = array(
			'lastip' => '192.168.2.254',
			'lastvisit' => $publishdate,
			'lastactivity' => $publishdate,
			'lastpost' => TIMESTAMP
	);
	C::t('common_member_status')->update($authorid, $last_status, 'UNBUFFERED');
	
	$subject = str_replace("\t", ' ', $_POST["subject"]);
	$lastpost = "$tid\t".$subject."\t$_G[timestamp]\t$author";
	C::t('forum_forum')->update($fid, array('lastpost' => $lastpost));
	C::t('forum_forum')->update_forum_counter($fid, 1, 1, 1);
	
	echo "OK, ID: ".$pid;
?>