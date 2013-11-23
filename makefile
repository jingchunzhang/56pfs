SUBDIRS = lib network network/N network/S network/D network/U network/POSS api/ php_client/
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

