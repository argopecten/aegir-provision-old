<?php

/**
 * Restore command implementation
 *
 * This command when called will 
 *   1. Make a backup of the current site, before modifications are made.
 *   2. Temporarily disable the site by causing apache to redirect to a help page. Restarting apache is required.
 *   3. Extract the backup that is being restored to to a temporary folder in the sites directory.
 *   4. Create a new database, belonging to the site's user, and switch it around with the current site's database.
 *   5. Import the old database and site.php details.
 *   6. Switch around the sites directory of the current site and the backup being restored.
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
 * Make sure we have a valid site being restored, and that the file being restored from exists
 */
function provision_drupal_provision_restore_validate($url = null, $restore_file = null) {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);

  $exists = provision_path("exists", $restore_file, TRUE,
      dt("Restoring site from @path"),
      dt("Could not find backup file @path"),
      'PROVISION_BACKUP_NOT_FOUND');
  if ($exists) {
    drush_set_option('restore_file', $restore_file);
  }
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function provision_drupal_provision_pre_restore($url, $restore_file) {
  provision_invoke('backup', $url);

  provision_path("extract", drush_get_option('restore_file'), drush_get_option('sites_path') ."/$url.restore", 
    dt('Successfully extracted the contents of @path'),
    dt('Failed to extract the contents of @path'),
    PROVISION_BACKUP_EXTRACTION_FAILED);
}

/**
 * Remove the extracted site directory
 */
function provision_drupal_provision_pre_restore_rollback($url) {
  _provision_recursive_delete(drush_get_option('sites_path') ."/$url.restore");
}

/**
 * Switch the restore directories around now that we have the new db installed
 */
function provision_drupal_provision_restore($url) {
  $old = drush_get_option('sites_path') ."/$url.restore";
  $new = drush_get_option('sites_path') ."/$url";
  provision_path("switch_paths", $old, $new ,
    dt('Swapping out the @path and @confirm directories was successful.'),
    dt('Swapping the @path and @confirm directories has failed.'),
    DRUSH_PERM_ERROR);
  // make sure it has the latest site data available
  _provision_drupal_create_settings_file($url);
  provision_save_site_data($url);
}

// Luckily this is reversable =)
function provision_drupal_provision_restore_rollback($url) {
  provision_drupal_provision_restore($url);
}

/**
 * Delete the old site directory and recreate the settings file
 */
function provision_drupal_provision_post_restore($url) {
  _provision_recursive_delete(drush_get_option('sites_path') ."/$url.restore");
  _provision_drupal_create_settings_file($url);
}

