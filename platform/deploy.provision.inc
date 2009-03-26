<?php

/**
 * Restore command implementation
 *
 * This command when called will 
 *   1. Make a backup of the current site, before modifications are made.
 *   2. Temporarily disable the site by causing apache to redirect to a help page. Restarting apache is required.
 *   3. Extract the backup that is being deployd to to a temporary folder in the sites directory.
 *   4. Create a new database, belonging to the site's user, and switch it around with the current site's database.
 *   5. Import the old database and site.php details.
 *   6. Switch around the sites directory of the current site and the backup being deployd.
 *   7. Regenerate configuration files.
 *   8. TODO: diagnostic to test that everything is ok?
 *   9. Remove the temporary redirect and restart apache so the previous site is available again.
 *   10. Remove the extranuous db and duplicate site directory.
 *
 * If at any time an error occurs, before step 9. It should reverse all the changes it has made,
 * and leave the current site directory and database in the right place, and remove all cruft that
 * was created by this process.
 */

/**
 * Make sure we have a valid site being deployd, and that the file being deployd from exists
 */
function provision_drupal_provision_deploy_validate($url = null, $backup_file = null) {
  _provision_drupal_url_required();

  $exists = provision_path("exists", $backup_file, TRUE,
      dt("Deploying site from @path"),
      dt("Could not find backup file @path"),
      'PROVISION_BACKUP_NOT_FOUND');
  if ($exists) {
    drush_set_option('backup_file', $backup_file);
  }
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function provision_drupal_provision_pre_deploy($url, $backup_file) {
  $extracted = provision_path("extract", drush_get_option('backup_file'), drush_get_option('sites_path') ."/$url", 
    dt('Successfully extracted the contents of @path'),
    dt('Failed to extract the contents of @path'),
    'PROVISION_BACKUP_EXTRACTION_FAILED');
  if ($extracted) {
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
    // @TODO Clear up this properly.
//    $old_data = provision_get_site_data($url);
//    $data = array_merge($old_data, $data); 
  }
}

/**
 * Remove the extracted site directory
 */
function provision_drupal_provision_pre_deploy_rollback($url) {
  _provision_recursive_delete(drush_get_option('sites_path') ."/$url");
}

/**
 * Switch the deploy directories around now that we have the new db installed
 */
function provision_drupal_provision_deploy($url) {
  // make sure it has the latest site data available
  _provision_drupal_create_settings_file($url);
}

// Luckily this is reversable =)
function provision_drupal_provision_deploy_rollback($url) {
  provision_drupal_provision_deploy($url);
}

function provision_drupal_provision_post_deploy($url) {
  provision_invoke('update', $url);
  provision_platform_include(dirname(__FILE__), 'deploy');
}