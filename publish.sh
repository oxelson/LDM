# Publishes the current LDM source distribution.
#   - Executes a local "make install"
#   - Copies the tarball to the public download directory
#   - Deletes old, bug-fix versions
#   - Copies the installed documentation on the LDM's website
#   - Delete old, bug-fix versions
#   - Adjusts symbolic links on the LDM's website
#
# Usage:
#       $0 <host>
# where:
#       <host>     Name of the host on which the LDM package is made public
#                  (e.g., "www.unidata.ucar.edu")

set -e  # exit on failure

host=${1?Host not specified}

# Ensure that the package is installed so that the documentation can be copied
# to the website.
echo Installing package locally
make install >&install.log

version=`awk 'NR==1{print $1; exit;}'
tarball=ldm-$version.tar.gz
ftpdir=/web/ftp/pub/software/ldm
webdir=/web/content/software/ldm

# Ensure that the FTP directory contains the tarball
if ! ssh $host test -e $ftpdir/$tarball; then
    #
    # Copy the tarball to the FTP directory.
    #
    echo Copying  $tarbal to $ftpdir
    trap "ssh $host rm -f $ftpdir/$tarball; `trap -p ERR`" ERR
    scp $tarball $host:$tarball
fi

# Purge the FTP directory of bug-fix versions that are older than the latest
# corresponding minor release.
echo Purging $ftpdir of bug-fixes older than $tarball
ssh -T $host bash --login <<EOF
    set -ex # Exit on error
    cd $ftpdir
    ls -d ldm-[0-9.]*.tar.gz |
        sed "s/ldm-//" |
        sort -t. -k 1nr,1 -k 2nr,2 -k 3nr,3 |
        awk -F. '\$1!=ma||\$2!=mi{print}{ma=\$1;mi=\$2}' >versions
    for vers in \`ls -d ldm-[0-9.]*.tar.gz | sed "s/ldm-//"\`; do
        fgrep -s \$vers versions || rm -rf ldm-\$vers
    done
EOF

# Copy the documentation to the package's website
versionWebDir=$webdir/ldm-$version
echo Copying documentation to $versionWebDir
ssh -T $host rm -rf $versionWebDir
trap "ssh -T $host rm -rf $versionWebDir; `trap -p ERR`" ERR
scp -Br ../share/doc/ldm $host:$versionWebDir

# Ensure that the package's home-page references the just-copied documentation.
ssh -T $host bash --login <<EOF
    set -ex  # Exit on error

    # Go to the top-level of the package's web-pages.
    cd $webdir

    # Allow group write access to all created files.
    umask 02

    # Set the hyperlink references to the documentation. For a given major
    # and minor version, keep only the latest bug-fix.
    echo Linking to documentation in $host:`pwd`
    ls -d ldm-[0-9.]*.tar.gz |
        sed "s/ldm-//" |
        sort -t. -k 1nr,1 -k 2nr,2 -k 3nr,3 |
        awk -F. '\$1!=ma||\$2!=mi{print}{ma=\$1;mi=\$2}' >versions
    sed -n '1,/BEGIN VERSION LINKS/p' versions.inc >versions.inc.new for vers in \`cat versions\`; do
        href=ldm-\$vers
        cat <<END_VERS >>versions.inc.new
             <tr>
              <td>
               <b>\$vers</b>
              </td>
              <td>
               <a href="\$href">Documentation</a> 
              </td>
              <td>
               <a href="ftp://ftp.unidata.ucar.edu/pub/ldm/\$href.tar.gz">Download</a> 
              </td>
               <td>
               <a href="\$href/CHANGE_LOG">Release Notes</a> 
              </td>
             </tr>

END_VERS
    done
    sed -n '/END VERSION LINKS/,\$p' versions.inc >>versions.inc.new
    rm -f versions.inc.old
    cp versions.inc versions.inc.old
    mv versions.inc.new versions.inc

    # Delete all versions not referenced in the top-level HTML file.
    echo Deleting unreferenced version in $host:`pwd`
    for vers in \`ls -d ldm-[0-9.]*.tar.gz | sed "s/ldm-//"\`; do
        fgrep -s \$vers versions || rm -rf ldm-\$vers
    done

    # Set the symbolic link to the current version
    echo Making ldm-$version the current version
    rm -f ldm-current
    ln -s ldm-$version ldm-current
EOF