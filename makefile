SUBDIRS = 3rdlib lib network network/N network/S network/D network/U network/POSS api/ php_client/
curday = $(shell date '+%Y%m%d')
installdir = /home/jingchun.zhang/3rd/nginx/html/pfs
all:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	echo "Making all in $$list"; \
	(cd $$subdir && make); \
	done;

clean:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	echo "Making all in $$list"; \
	(cd $$subdir && make clean); \
	done;

install:
	rm -rf /diska/pfs/*;
	mkdir /diska/pfs/bin -p;
	mkdir /diska/pfs/log -p;
	mkdir /diska/pfs/conf -p;
	mkdir /diska/pfs/data -p;
	mkdir /diska/pfs/path/tmpdir -p;
	mkdir /diska/pfs/path/outdir -p;
	mkdir /diska/pfs/path/datadir -p;
	mkdir /diska/pfs/path/workdir -p;
	mkdir /diska/pfs/path/indir -p;
	mkdir /diska/pfs/path/bkdir -p;
	mkdir /diska/pfs/path/delfile -p;
	cd network; cp pfs_master /diska/pfs/bin; cp pfs_master.conf /diska/pfs/conf; cp *.txt /diska/pfs/bin;
	cd network/N; cp *.so /diska/pfs/bin
	cd network/S; cp *.so /diska/pfs/bin
	cd network/D; cp *.so /diska/pfs/bin
	cd network/U; cp *.so /diska/pfs/bin
	cd network/POSS; cp *.so /diska/pfs/bin
	cp script/*.sh /diska/pfs/bin

img:
	cd network; cp pfs_master.conf.img /diska/pfs/conf/pfs_master.conf
	cd $(installdir); mv pfs_img.tgz pfs_img_$(curday).tgz;
	cd /diska/; tar -czf $(installdir)/pfs_img.tgz pfs/;
v:
	cd network; cp pfs_master.conf.v /diska/pfs/conf/pfs_master.conf
	cd $(installdir); mv pfs_v.tgz pfs_v_$(curday).tgz;
	cd /diska/; tar -czf $(installdir)/pfs_v.tgz pfs/;
vxinxing:
	cd network; cp pfs_master.conf.vxinxing /diska/pfs/conf/pfs_master.conf
	cd $(installdir); mv pfs_vxinxing.tgz pfs_vxinxing_$(curday).tgz;
	cd /diska/; tar -czf $(installdir)/pfs_vxinxing.tgz pfs/;
m:
	cd network; cp pfs_master.conf.m /diska/pfs/conf/pfs_master.conf
	cd $(installdir); mv pfs_m.tgz pfs_m_$(curday).tgz;
	cd /diska/; tar -czf $(installdir)/pfs_m.tgz pfs/;
testpfs:
	cd network; cp pfs_master.conf.testpfs /diska/pfs/conf/pfs_master.conf
	cd $(installdir); mv pfs_testpfs.tgz pfs_testpfs_$(curday).tgz;
	cd /diska/; tar -czf $(installdir)/pfs_testpfs.tgz pfs/;

