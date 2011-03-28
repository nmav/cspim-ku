PACKAGE=cspim-1.0
mkdir $PACKAGE
find . \
	-name $PACKAGE -prune \
	-o -name .svn -prune \
	-o -name doc -prune \
	-o -print | cpio -p $PACKAGE
tar czvf ${PACKAGE}.tar.gz $PACKAGE
rm -rf $PACKAGE
