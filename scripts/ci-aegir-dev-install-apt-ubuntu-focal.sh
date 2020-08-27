#
# Install Aegir debian packages located in the 'build/' directory.
# These are provided by the GitLab CI build stage.
#
# This script is tuned for Ubuntu 20.04. (focal)
#
echo "[CI] Updating APT"
sudo apt update

echo "[CI] Setting debconf settings"
sudo debconf-set-selections <<EOF
debconf debconf/frontend select Noninteractive
aegir3-hostmaster aegir/db_password string PASSWORD
aegir3-hostmaster aegir/db_password seen true
aegir3-hostmaster aegir/db_user string aegir_root
aegir3-hostmaster aegir/db_host string localhost
aegir3-hostmaster aegir/email string aegir@example.com
aegir3-hostmaster aegir/site string aegir.example.com
postfix postfix/main_mailer_type select Local only
aegir3-provision aegir/drush_version string 8.3.4
EOF

echo "[CI] Pre-installing dependencies"
sudo apt install --yes mysql-server mysql-client

echo "[CI] Installing .deb files .. will fail on missing packages"
sudo DPKG_DEBUG=developer apt install --yes ./build/aegir3_*.deb ./build/aegir3-provision*.deb ./build/aegir3-hostmaster*.deb

echo "[CI] Installing remaining packages and configuring our debs"
sudo DPKG_DEBUG=developer apt install --fix-broken --yes
