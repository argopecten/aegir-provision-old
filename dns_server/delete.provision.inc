<?php

// $Id$

/**
 * @file
 *   Provision hooks for the delete command
 **/

/**
 * Implementation of hook_provision_delete()
 *
 * delete the RR for this site at least, but possibly also the zone itself?
 * need to lookup the zid for the base zone of this site, and then call provision_dns_rr with the zid and site 'name'
 * (ie: pull off the tld and second-level domain for the 'base' zone, and treat the rest as the 'name')
 */
function provision_dns_provision_delete($url = NULL) {
  if ($url) {
#    provision_dns_delete($url);
  }
}
