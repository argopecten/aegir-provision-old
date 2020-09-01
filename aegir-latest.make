core = 7.x
api = 2

projects[drupal][type] = "core"

; Pin a core version, only as long as we have a core patch below.
; Sync manually with drupal-org-core.make in the hostmaster repository.
projects[drupal][version] = 7.72

; Issue #3168480: Mysql 8 Support on Drupal 7 (https://www.drupal.org/project/drupal/issues/2978575)
projects[drupal][patch][2978575] = "https://www.drupal.org/files/issues/2020-08-14/2978575-218.patch"

projects[hostmaster][type] = "profile"
projects[hostmaster][version] = "3"
