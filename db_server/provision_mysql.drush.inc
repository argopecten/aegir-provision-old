<?php
// $Id$
/**
 * @file
 *    Mysql provisioning module.
 *
 * The goal of this module is to create mysql databases and user accounts, for sites that are about to be created.
 * It uses the provision API to tie into the right places in the site creation work flow.
 */
include_once('provision.mysql.inc');

function provision_mysql_provision_init() {
  drush_set_default('master_db', $GLOBALS['db_url']);
  $master_db = drush_get_option('master_db');
  $db = parse_url($master_db);
  drush_set_default('master_db_user', $db['user']);
  drush_set_default('master_db_passwd', $db['pass']);

  drush_set_default('master_db_host', $db['host']);
  drush_set_default('db_host', $db['host']);
 
  drush_set_default('master_db_type', $db['scheme']);
  drush_set_default('db_type', $db['scheme']);
}

function provision_mysql_provision_finalize() {
  provision_db_close();
}
function provision_mysql_drush_help($section) {
  switch ($section) {
    case 'error:PROVISION_CREATE_DB_FAILED' :
      return dt('Unable to create new databases.');
    case 'error:PROVISION_DROP_DB_FAILED' :
      return dt('Unable to drop database.');
  }

}
/**
 * Generate a new mysql database and user account for the specified credentials
 */
function _provision_mysql_new_site_db($db_name, $db_user, $db_passwd, $db_host) {
  if (!_provision_mysql_create_database($db_name) ||
      !_provision_mysql_database_exists($db_name) ) {
   drush_set_error('PROVISION_CREATE_DB_FAILED');
   drush_log("Database could not be created.", 'error');
   return FALSE;
  }
  
  if (!_provision_mysql_grant($db_name, $db_user, $db_passwd)
      || !_provision_mysql_grant($db_name, $db_user, $db_passwd, 'localhost')) {
    drush_log("Could not GRANT user access.", 'warning');
  }
  _provision_mysql_flush_privileges();

  $status = _provision_mysql_database_exists($db_name);

  if ($status) {
    drush_log(dt('Created @name database', array("@name" => $db_name)), 'success');
  }
  else {
    drush_set_error('PROVISION_CREATE_DB_FAILED', dt("Could not create @name database", array("@name" => $db_name)));
  }
  return $status; 
   //TODO : Test to confirm that the database is actually writeable. Taking this on faith for now.
}

/**
 * Remove the database and user account for the supplied credentials
 */
function _provision_mysql_destroy_site_db($db_name, $db_user, $db_passwd, $db_host) {
    
  if ( _provision_mysql_database_exists($db_name) ) {
    drush_log(dt("Dropping database @dbname", array('@dbname' => $db_name)));
    if (!_provision_mysql_drop_database($db_name)) {
      drush_log(dt("Failed to drop database @dbname", array('@dbname' => $db_name)), 'warning');
    }
  }
  
  if ( _provision_mysql_database_exists($db_name) ) {
   drush_set_error('PROVISION_DROP_DB_FAILED');
   return FALSE;
  }
  
  drush_log("Revoking privileges");
  _provision_mysql_flush_privileges();
  if (!_provision_mysql_revoke($db_name, $db_user)) {
      drush_log(dt("Failed to revoke user privileges"), 'warning');
  }
}


function _provision_mysql_database_exists($name) {
  return provision_db_result(provision_db_query("SHOW DATABASES LIKE '%s'", $name));
}

function _provision_mysql_drop_database($name) {
  return provision_db_query("DROP DATABASE `%s`", $name);
}

function _provision_mysql_create_database($name) {
  return provision_db_query("CREATE DATABASE %s", $name);  
}

function _provision_mysql_flush_privileges() {
  return provision_db_query("FLUSH PRIVILEGES");  
}

function _provision_mysql_can_create_database() {
  $test = 'provision_test';
  _provision_mysql_create_database($test);
  if (_provision_mysql_database_exists($test)) {
    if (!_provision_mysql_drop_database($test)) {
      drush_log(dt("Failed to drop database @dbname", array('@dbname' => $test)), 'warning');
    }
    return TRUE;
  }
  return FALSE;
}

function _provision_mysql_grant($name, $username, $password, $host = '') {
  $host = ($host) ? $host : '%';
  return provision_db_query("GRANT ALL PRIVILEGES ON `%s`.* TO `%s`@`%s` IDENTIFIED BY '%s'", $name, $username, $host, $password);
}

function _provision_mysql_revoke($name, $username, $host = '') {
  $host = ($host) ? $host : '%';
  return provision_db_query("REVOKE ALL PRIVILEGES ON `%s`.* FROM `%s`@`%s`", $name, $username, $host);
}

function _provision_mysql_import_dump($dump_file, $db_name, $db_user, $db_passwd, $db_host) {
  $exists = provision_path("exists", $dump_file, TRUE,
    dt('Found database dump at @path.'),
    dt('No database dump was found at @path.'),
    PROVISION_DB_DUMP_NOT_FOUND);
  if ($exists) {
    $readable = provision_path("readable", $dump_file, TRUE, dt('Database dump at @path is readable'), 
      dt('The database dump at @path could not be read.'), 
      PROVISION_DB_DUMP_NOT_READABLE);
    if ($readable) {
      drush_log(sprintf("Importing database using command: mysql -u%s -p%s -h%s %s < %s", 
        $db_user, $db_passwd, $db_host, $db_name, $dump_file));
      if (!provision_shell_exec("mysql -u%s -p%s -h%s %s < %s", $db_user, $db_passwd, $db_host, $db_name, $dump_file )) {
        drush_set_error('PROVISION_DB_IMPORT_FAILED', dt("Database import failed"));
      }
     }
   }
}

/**
 * Find a viable database name, based on available information.
 *
 * This function exists solely to work past mysql's database name restrictions.
 * As mysql also does not have the ability to rename databases, it is completely
 * possible that sites will be running with derivative names on the same server,
 * until the upgrade / restore process is completed.
 */ 
function _provision_mysql_suggest_db_name($url) {
  if ($sid = drush_get_option('site_id')) {
    $id_suggest = 'site_'. $sid;
    $suggest[] = $id_suggest;
    for ($i = 0; $i < 100; $i++) {
      $suggest[] = $id_suggest .'_'. $i;
    }
  }
  else {
    // This is a last option, and not ideal
    // Provision only users will trigger this mostly.
    $url_suggest = substr(str_replace(array(".", "-"), '' , ereg_replace("^www\.", "", $url)), 0, 14);
    $suggest[] = $url_suggest;
    for ($i = 0; $i < 100; $i++) {
      $suggest[] = $url_suggest .'_'. $i;
    }
  }

  foreach ($suggest as $option) {
    if (!_provision_mysql_database_exists($option)) {
      return $option;
    }
  }

}